#include "GPIO.h"
#include "PWM.h"
#include <thread> // For std::this_thread::sleep_for

int
main()
{
  enum class BlinkMethod
  {
    SLEEP,
    PWM
  };

  constexpr int gpioPin = 55; /* Specify your GPIO pin here */
  constexpr BlinkMethod method{ BlinkMethod::SLEEP };

  if (method == BlinkMethod::SLEEP) {

    constexpr size_t nanoSecondsOn = 1000;
    constexpr size_t nanoSecondsOff = 1000;

    if (!GPIO::exportGPIO(gpioPin)) {
      std::cerr << "Failed to export GPIO pin " << gpioPin << std::endl;
      return -1;
    }

    // Set GPIO direction to "out"
    if (!GPIO::setDirection(gpioPin, GPIO::Direction::OUT)) {
      std::cerr << "Failed to set direction on GPIO pin " << gpioPin
                << std::endl;
      return -1;
    }

    auto gpioValueFile = GPIO::openGPIOValueFile(gpioPin);
    while (true) {
      gpioValueFile << "1" << std::flush;
      std::this_thread::sleep_for(std::chrono::nanoseconds(nanoSecondsOn));
      gpioValueFile << "0" << std::flush;
      std::this_thread::sleep_for(std::chrono::nanoseconds(nanoSecondsOff));
    }
    gpioValueFile.close();

    // Unexport GPIO pin
    GPIO::exportGPIO(gpioPin, true);
  } else {

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
  }

  return 0;
}
