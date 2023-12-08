#include "PWM.h"

int
main()
{
  constexpr size_t periodInNs = 200;
  constexpr size_t dutyCycleInNs = 100;

  constexpr int chipNr = 11;
  constexpr int pwmNr = 0;
  constexpr auto polarity = PWM::Polarity::NORMAL;

  PWM::exportPWM(chipNr, pwmNr);
  PWM::setPeriodInNs(chipNr, pwmNr, periodInNs);
  PWM::setDutyCycleInNs(chipNr, pwmNr, dutyCycleInNs);
  PWM::setPolarity(chipNr, pwmNr, polarity);
  PWM::setEnabled(chipNr, pwmNr, true);
  while (true) {
    ;
  }
  PWM::setEnabled(chipNr, pwmNr, false);
  PWM::exportPWM(chipNr, pwmNr, true);

  return 0;
}
