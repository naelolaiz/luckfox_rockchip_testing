#include "GPIO.h"
#include "PWM.h"

#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

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
      throw std::runtime_error("invalid value");
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
      << " [--help] [--sleepMs 100] [--updateStep 0.1] "
         "[--servoPitchFreq 1.0] [--servoPitchIP 0.0] [--servoPitchMinValInNs "
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
  size_t cyclesForSpiral = 0; // 0 = no changes

  size_t updateSleepInMs = 100u;
  float updateStep = 0.1f;
  float servoPitchFreq = 1.f;
  float servoYawFreq = 2.f;
  float servoPitchInitialNormalizedPhase = 0.f;
  float servoYawInitialNormalizedPhase = 0.25f;
  bool enableLaser = false;
  int gpioNrForLaser = DEFAULT_GPIO_NR_FOR_LASER;

  constexpr auto twoPi = 2 * M_PI;

  if (argc > 1) {

    for (size_t i = 1; i < argc; i++) {
      const auto currentArg = std::string(argv[i]);
      if (currentArg == "--help") {
        printHelp();
        return 0;
      } else if (currentArg == "--sleepMs") {
        updateSleepInMs = atoi(argv[++i]);
      } else if (currentArg == "--updateStep") {
        updateStep = atof(argv[++i]);
      } else if (currentArg == "--servoPitchFreq") {
        servoPitchFreq = atof(argv[++i]);
      } else if (currentArg == "--servoYawFreq") {
        servoYawFreq = atof(argv[++i]);
      } else if (currentArg == "--servoPitchIP") {
        servoPitchInitialNormalizedPhase = atof(argv[++i]);
      } else if (currentArg == "--servoYawIP") {
        servoYawInitialNormalizedPhase = atof(argv[++i]);
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
      } else if (currentArg == "--spiral") {
        cyclesForSpiral = atoi(argv[++i]);
      } else {
        printHelp();
        return -1;
      }
    }
  }
  std::signal(SIGINT, signalHandler);

  const auto servoPitchInitialPhase = servoPitchInitialNormalizedPhase * twoPi;
  const auto servoYawInitialPhase = servoYawInitialNormalizedPhase * twoPi;

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

  size_t currentCycle = 0;
  double currentGain = cyclesForSpiral != 0 ? 1. : 0.;
  while (!signalReceived) {

    if (cyclesForSpiral != 0) {
      currentGain = static_cast<double>(currentCycle) / cyclesForSpiral;
    }
    for (float f = 0.f; f < twoPi; f += updateStep) {
      if (signalReceived) {
        break;
      }
      servoPitch.setValue(currentGain *
                          sin(servoPitchInitialPhase + f * servoPitchFreq));
      servoYaw.setValue(currentGain *
                        sin(servoYawInitialPhase + f * servoYawFreq));
      std::this_thread::sleep_for(std::chrono::milliseconds(updateSleepInMs));
    }
    if (cyclesForSpiral != 0 && ++currentCycle == cyclesForSpiral) {
      currentCycle = 0;
    }
  }
  if (enableLaser) {
    laserPointer.disable();
  }

  return 0;
}
