#include "GPIO.h"
#include "PWM.h"

#include "svg/nael.h"

#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <tuple>
#include <vector>

// class GraphChars
//{
// public:
//   GraphChars()
//   {
//     constexpr float totalWidth = 2.f;
//     const float charWidth = totalWidth / mCharsCount;
//     const float halfCharWidth = charWidth / 2.f;
//     float currentX = -1.f;
//     float currentY = 0.f;
//     size_t charNr = 0;
//     for (auto v : mCharsMovements) {
//       size_t counter = 0;
//       for (auto t : v) {
//         const auto steps = std::get<0>(t);
//         const auto scaledX = std::get<1>(t) / mCharsCount;
//         const auto finalX = scaledX + halfCharWidth - 1.f;
//         const auto finalY = std::get<2>(t);
//         const auto ledValue = std::get<3>(t);
//         const auto stepSizeX = (finalX - currentX) / steps;
//         const auto stepSizeY = (finalY - currentY) / steps;
//         for (auto i = 0; i < steps; i++) {
//           currentX += stepSizeX;
//           currentY += stepSizeY;
//           mInterpolatedValues.push_back(currentX, currentY, ledValue);
//         }
//         currentX = finalX;
//         currentY = finalY;
//         counter++;
//         // set laser(std::get<3>(t));
//       }
//       //    mInterpolatedValues.resize(counter);
//       charNr++;
//     }
//   }
//   std::tuple<float, float> getNextXY()
//   {
//     std::tuple<float, float> toReturn;
//     size_t counter = 0; // TODO: optimize
//     for (auto v : mCharsMovements) {
//       for (auto t : v) {
//         if (counter)
//           counter++;
//       }
//     }
//     return toReturn;
//   }

// private:
//   enum class CurrentStatus
//   {
//     WAITING_AND_INTERPOLATING,
//     SET_LAST_VALUE_AND_READ_NEXT
//   };
//   const std::vector<
//     std::vector<std::tuple<size_t /*cycles from current to this position*/,
//                            float /*x after cycles: -1.f/1.f*/,
//                            float /*y after cycles*/,
//                            bool /*state after cycles*/
//                            >>>
//     mCharsMovements{ /*H*/
//                      { { 1u, -1.f, 1.f, true },
//                        { 10u, -1.f, -1.f, true },
//                        { 5u, -1.f, 0.f, true },
//                        { 6u, 1.f, 0.f, true },
//                        { 5u, 1.f, 1.f, true },
//                        { 10u, 1.f, -1.f, false } },
//                      /*E*/
//                      { { 1u, 1.f, 1.f, true },
//                        { 5u, -1.f, 1.f, true },
//                        { 5u, -1.f, 0.f, true },
//                        { 4u, 0.75f, 0.f, true },
//                        { 5u, -1.f, 0.f, true },
//                        { 5u, -1.f, -1.f, true },
//                        { 5u, 1.f, -1.f, false } },
//                      /*L*/
//                      { { 1u, -1.f, 1.f, true },
//                        { 10u, -1.f, -1.f, true },
//                        { 5u, 1.f, -1.f, false } },
//                      /*L*/
//                      { { 1u, -1.f, 1.f, true },
//                        { 10u, -1.f, -1.f, true },
//                        { 5u, 1.f, -1.f, false } },
//                      /*O*/
//                      { { 1u, -1.f, 1.f, true },
//                        { 10u, -1.f, -1.f, true },
//                        { 10u, 1.f, -1.f, true },
//                        { 10u, 1.f, 1.f, true },
//                        { 10u, -1.f, 1.f, false } }
//     };
//   const size_t mCharsCount = mCharsMovements.size();
//   size_t mCurrrentPosition{};
//   CurrentStatus mCurrentStatus{ CurrentStatus::SET_LAST_VALUE_AND_READ_NEXT
//   }; std::vector<std::tuple<float, float, bool /*laser state at end*/>>
//     mInterpolatedValues;
// };

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
  size_t cyclesForSpiral = 0; // 0 = no changes

  constexpr size_t updateSleepInMs = 20u; // 20 ms sleeps for ~50Hz updates
  float updateStep = 0.05f;
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
    for (auto path : nael) {
      if (enableLaser) {
        laserPointer.disable();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(waitBetweenChars));
      if (enableLaser) {
        laserPointer.enable();
      }
      for (std::pair<double, double> x_y : path) {
        servoYaw.setValue(std::get<0>(x_y));
        servoPitch.setValue(std::get<1>(x_y));
        std::this_thread::sleep_for(std::chrono::milliseconds(updateSleepInMs));
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(waitBetweenCycle));
  }
  if (enableLaser) {
    laserPointer.disable();
  }
  return 0;
}
