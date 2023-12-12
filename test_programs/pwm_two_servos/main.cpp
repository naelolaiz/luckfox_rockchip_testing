#include "GPIO.h"
#include "PWM.h"

#include "svg/hello_world.h"
#include "svg/nael.h"

#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <tuple>
#include <vector>

volatile sig_atomic_t signalReceived = 0;
void
signalHandler(int signal)
{
  signalReceived = 1;
}

class Servo
{
public:
  Servo(int chipNr,
        int pwmNr,
        size_t minDutyCycleInNs = ABSOLUTE_MIN_DUTY_CYCLE_IN_NS,
        size_t maxDutyCycleInNs = ABSOLUTE_MAX_DUTY_CYCLE_IN_NS)
    : mChipNr(chipNr)
    , mPwmNr(pwmNr)
    , mMinDutyCycleInNs(minDutyCycleInNs)
    , mMaxDutyCycleInNs(maxDutyCycleInNs)
    , mDutyCycleMidRange((mMaxDutyCycleInNs - mMinDutyCycleInNs) / 2u)
  {
    if (mMinDutyCycleInNs < ABSOLUTE_MIN_DUTY_CYCLE_IN_NS ||
        mMaxDutyCycleInNs > ABSOLUTE_MAX_DUTY_CYCLE_IN_NS) {
      throw std::runtime_error("???");
    }
  }

  ~Servo()
  {
    if (mStarted) {
      PWM::setEnabled(mChipNr, mPwmNr, false);
      PWM::exportPWM(mChipNr, mPwmNr, true);
    }
  }

  void setupAndStart()
  {
    PWM::exportPWM(mChipNr, mPwmNr);
    PWM::setPeriodInNs(mChipNr, mPwmNr, FIFTY_HZ_PERIOD_IN_NS);
    setValue(0.f);
    PWM::setPolarity(mChipNr, mPwmNr, PWM::Polarity::NORMAL);
    PWM::setEnabled(mChipNr, mPwmNr, true);
    mStarted = true;
  }

  void setValue(float value) // -1/+1 (0=center)
  {
    if (value < -1.f || value > 1.f) {
      throw std::runtime_error(std::string("invalid value ") +
                               std::to_string(value));
    }
    mCurrentValue = value;
    auto dutyCycleInNs = mMinDutyCycleInNs + (1 + value) * mDutyCycleMidRange;
    PWM::setDutyCycleInNs(mChipNr, mPwmNr, dutyCycleInNs);
  }

private:
  static constexpr size_t FIFTY_HZ_PERIOD_IN_NS = 1E9 / 50u; // 50Hz period
  float mCurrentValue{ 0.f };
  const size_t mChipNr;
  const size_t mPwmNr;
  bool mStarted{ false };
  const size_t mMinDutyCycleInNs;
  const size_t mMaxDutyCycleInNs;
  const size_t mDutyCycleMidRange;

public:
  static constexpr size_t ABSOLUTE_MIN_DUTY_CYCLE_IN_NS =
    FIFTY_HZ_PERIOD_IN_NS / 50u; // 2%
  static constexpr size_t ABSOLUTE_MAX_DUTY_CYCLE_IN_NS =
    6u * FIFTY_HZ_PERIOD_IN_NS / 50u; // 12%
};

class LaserPointer
{
public:
  LaserPointer(int gpioNr)
    : mGpioNr(gpioNr)
  {
  }
  ~LaserPointer()
  {
    if (mStarted) {
      disable();
    }
  }
  bool enable()
  {
    if (!GPIO::exportGPIO(mGpioNr)) {
      std::cerr << "Failed to export GPIO pin " << mGpioNr << std::endl;
      return false;
    }

    // Set GPIO direction to "out"
    if (!GPIO::setDirection(mGpioNr, GPIO::Direction::OUT)) {
      std::cerr << "Failed to set direction on GPIO pin " << mGpioNr
                << std::endl;
      return false;
    }

    mValueFileStream = GPIO::openGPIOValueFile(mGpioNr);
    mStarted = true;
    return true;
  }
  bool disable()
  {
    setValue(false);
    mValueFileStream.close();
    if (!GPIO::exportGPIO(mGpioNr, true)) {
      std::cerr << "Failed to unexport GPIO pin " << mGpioNr << std::endl;
      return false;
    }
    mStarted = false;
    return true;
  }

  void setValue(bool value)
  {
    mValueFileStream << (value ? "1" : "0") << std::flush;
  }

private:
  const int mGpioNr;
  bool mStarted{ false };
  std::ofstream mValueFileStream;
};

int
main(int argc, char* argv[])
{
  auto printHelp = [&argv]() {
    std::cout
      << " Usage: " << argv[0]
      << " [--help] [--updateStep 0.05] "
         "[--servoPitchFreq 1.0] [--servoPitchIP 0.0] "
         "[--servoPitchMinValInNs "
         "0.02*1e9/50=400000] [--servoPitchMaxValInNs 0.12*1e9/50=2400000] "
         "[--servoYawFreq "
         "2.0] [--servoYawIP 0.25] [--servoYawMinValInNs 400000] "
         "[--servoYawMaxValInNs 2400000] [--enableLaser 4] [--spiral "
         "cycles_to_reach_max_val]"
      << std::endl;
  };

  constexpr int DEFAULT_GPIO_NR_FOR_LASER = 4;
  constexpr size_t SERVO_PITCH_PWM_CHIP_NR = 10;
  constexpr size_t SERVO_PITCH_PWM_NR = 0;
  constexpr size_t SERVO_YAW_PWM_CHIP_NR = 11;
  constexpr size_t SERVO_YAW_PWM_NR = 0;

  size_t servoYawMinNsDutyCycle = Servo::ABSOLUTE_MIN_DUTY_CYCLE_IN_NS;
  size_t servoYawMaxNsDutyCycle = Servo::ABSOLUTE_MAX_DUTY_CYCLE_IN_NS;
  size_t servoPitchMinNsDutyCycle = Servo::ABSOLUTE_MIN_DUTY_CYCLE_IN_NS;
  size_t servoPitchMaxNsDutyCycle = Servo::ABSOLUTE_MAX_DUTY_CYCLE_IN_NS;

  constexpr size_t updateSleepInMs = 20u; // 20 ms sleeps for ~50Hz updates
  bool enableLaser = false;
  int gpioNrForLaser = DEFAULT_GPIO_NR_FOR_LASER;

  constexpr auto twoPi = 2 * M_PI;

  if (argc > 1) {

    for (size_t i = 1; i < argc; i++) {
      const auto currentArg = std::string(argv[i]);
      if (currentArg == "--help") {
        printHelp();
        return 0;
      } else if (currentArg == "--enableLaser") {
        gpioNrForLaser = atoi(argv[++i]);
        enableLaser = true;
      } else if (currentArg == "--servoPitchMinValInNs") {
        servoPitchMinNsDutyCycle = atoi(argv[++i]);
      } else if (currentArg == "--servoPitchMaxValInNs") {
        servoPitchMaxNsDutyCycle = atoi(argv[++i]);
      } else if (currentArg == "--servoYawMinValInNs") {
        servoYawMinNsDutyCycle = atoi(argv[++i]);
      } else if (currentArg == "--servoYawMaxValInNs") {
        servoYawMaxNsDutyCycle = atoi(argv[++i]);
      } else {
        printHelp();
        return -1;
      }
    }
  }
  std::signal(SIGINT, signalHandler);

  const auto waitBetweenChars = updateSleepInMs * 30;
  const auto waitBetweenCycle = updateSleepInMs * 100;

  LaserPointer laserPointer(gpioNrForLaser);
  auto servoPitch = Servo(SERVO_PITCH_PWM_CHIP_NR,
                          SERVO_PITCH_PWM_NR,
                          servoPitchMinNsDutyCycle,
                          servoPitchMaxNsDutyCycle);
  auto servoYaw = Servo(SERVO_YAW_PWM_CHIP_NR,
                        SERVO_YAW_PWM_NR,
                        servoYawMinNsDutyCycle,
                        servoYawMaxNsDutyCycle);

  servoPitch.setupAndStart();
  servoYaw.setupAndStart();
  if (enableLaser) {
    laserPointer.enable();
    laserPointer.setValue(true);
  }

  while (!signalReceived) {
    for (auto svg : { hello_world, nael }) {
      for (auto path : svg) {
        if (enableLaser) {
          laserPointer.setValue(false);
        }
        std::this_thread::sleep_for(
          std::chrono::milliseconds(waitBetweenChars));
        bool laserOn = false;
        for (std::pair<double, double> x_y : path) {
          servoYaw.setValue(std::get<0>(x_y));
          servoPitch.setValue(std::get<1>(x_y));
          if (enableLaser && !laserOn) {
            laserPointer.setValue(true);
            laserOn = true;
          }
          std::this_thread::sleep_for(
            std::chrono::milliseconds(updateSleepInMs));
        }
      }
      laserPointer.setValue(false);
      std::this_thread::sleep_for(std::chrono::milliseconds(waitBetweenCycle));
    }
  }
  if (enableLaser) {
    laserPointer.disable();
  }
  return 0;
}
