#include "PWM.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>

class Servo
{
public:
  Servo(int chipNr, int pwmNr)
    : mChipNr(chipNr)
    , mPwmNr(pwmNr)
  {
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
    auto dutyCycleInNs =
      MIN_DUTY_CYCLE_IN_NS + (1 + value) * DUTY_CYCLE_MID_RANGE;
    PWM::setDutyCycleInNs(mChipNr, mPwmNr, dutyCycleInNs);
  }

private:
  static constexpr size_t FIFTY_HZ_PERIOD_IN_NS = 1E9 / 50u; // 50Hz period
  static constexpr size_t MIN_DUTY_CYCLE_IN_NS =
    FIFTY_HZ_PERIOD_IN_NS / 50u; // 2%
  static constexpr size_t MAX_DUTY_CYCLE_IN_NS =
    6u * FIFTY_HZ_PERIOD_IN_NS / 50u; // 12%
  static constexpr size_t DUTY_CYCLE_MID_RANGE =
    (MAX_DUTY_CYCLE_IN_NS - MIN_DUTY_CYCLE_IN_NS) / 2u;
  float mCurrentValue{ 0.f };
  const size_t mChipNr;
  const size_t mPwmNr;
  bool mStarted{ false };
};

int
main(int argc, char* argv[])
{
  auto printHelp = [&argv]() {
    std::cout << " Usage: " << argv[0]
              << " [--help] [--sleepMs 100] [--updateStep 0.1] "
                 "[--servo1Freq 1.0] [--servo2Freq 1.0]"
              << std::endl;
  };

  size_t updateSleepInMs = 100u;
  float updateStep = 0.1f;
  float servo1Freq = 1.f;
  float servo2Freq = 2.f;

  if (argc > 1) {

    for (size_t i = 1; i < argc; i++) {
      if (argv[i] == "--help") {
        printHelp();
        return 0;
      } else if (argv[i] == "--sleepMs") {
        updateSleepInMs = atoi(argv[++i]);
      } else if (argv[i] == "--updateStep") {
        updateStep = atof(argv[++i]);
      } else if (argv[i] == "--servo1Freq") {
        servo1Freq = atof(argv[++i]);
      } else if (argv[i] == "--servo2Freq") {
        servo2Freq = atof(argv[++i]);
      } else {
        printHelp();
        return -1;
      }
    }
  }

  auto servo1 = Servo(10, 0);
  auto servo2 = Servo(11, 0);

  servo1.setupAndStart();
  servo2.setupAndStart();

  while (true) {
    for (float f = 0.f; f < 360; f += updateStep) {
      servo1.setValue(sin(f * servo1Freq));
      servo2.setValue(cos(f * servo2Freq));
      std::this_thread::sleep_for(std::chrono::milliseconds(updateSleepInMs));
    };
  }

  return 0;
}
