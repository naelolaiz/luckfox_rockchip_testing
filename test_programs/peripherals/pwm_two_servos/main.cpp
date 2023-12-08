#include "PWM.h"
#include <cmath>
#include <thread> // For std::this_thread::sleep_for

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
main()
{

  auto servo1 = Servo(10, 0);
  auto servo2 = Servo(11, 0);

  servo1.setupAndStart();
  servo2.setupAndStart();

  while (true) {
    for (float f = 0.f; f < 360; f += .1) {
      servo1.setValue(sin(f));
      servo2.setValue(cos(f));
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    };
  }

  return 0;
}
