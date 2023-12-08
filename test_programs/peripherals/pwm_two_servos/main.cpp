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
    std::cout
      << " Usage: " << argv[0]
      << " [--help] [--sleepMs 100] [--updateStep 0.1] "
         "[--servoPitchFreq 1.0] [--servoYawFreq 2.0] [--servoPitchIP 0.0] "
         "[--servoYawIP 0.25]"
      << std::endl;
  };

  size_t updateSleepInMs = 100u;
  float updateStep = 0.1f;
  float servoPitchFreq = 1.f;
  float servoYawFreq = 2.f;
  float servoPitchInitialNormalizedPhase = 0.f;
  float servoYawInitialNormalizedPhase = 0.25f;

  constexpr auto twoPi = 2 * M_PI;

  if (argc > 1) {

    for (size_t i = 1; i < argc; i++) {
      if (argv[i] == "--help") {
        printHelp();
        return 0;
      } else if (std::string(argv[i]) == "--sleepMs") {
        updateSleepInMs = atoi(argv[++i]);
      } else if (std::string(argv[i]) == "--updateStep") {
        updateStep = atof(argv[++i]);
      } else if (std::string(argv[i]) == "--servoPitchFreq") {
        servoPitchFreq = atof(argv[++i]);
      } else if (std::string(argv[i]) == "--servoYawFreq") {
        servoYawFreq = atof(argv[++i]);
      } else if (std::string(argv[i]) == "--servoPitchIP") {
        servoPitchInitialNormalizedPhase = atof(argv[++i]);
      } else if (std::string(argv[i]) == "--servoYawIP") {
        servoYawInitialNormalizedPhase = atof(argv[++i]);
      } else {
        printHelp();
        return -1;
      }
    }
  }
  const auto servoPitchInitialPhase = servoPitchInitialNormalizedPhase * twoPi;
  const auto servoYawInitialPhase = servoYawInitialNormalizedPhase * twoPi;

  auto servoPitch = Servo(10, 0);
  auto servoYaw = Servo(11, 0);

  servoPitch.setupAndStart();
  servoYaw.setupAndStart();

  while (true) {
    for (float f = 0.f; f < twoPi; f += updateStep) {
      servoPitch.setValue(sin(servoPitchInitialPhase + f * servoPitchFreq));
      servoYaw.setValue(sin(servoYawInitialPhase + f * servoYawFreq));
      std::this_thread::sleep_for(std::chrono::milliseconds(updateSleepInMs));
    };
  }

  return 0;
}
