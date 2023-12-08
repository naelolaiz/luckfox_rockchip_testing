#include "GPIO.h"
#include <thread> // For std::this_thread::sleep_for

int
main()
{

  constexpr int gpioPin = 55; /* Specify your GPIO pin here */
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
  return 0;
}
