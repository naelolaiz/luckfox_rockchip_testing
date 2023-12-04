#include <cerrno>  // For errno
#include <cstdlib> // For system
#include <cstring> // For strerror
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread> // For std::this_thread::sleep_for

class GPIO
{
public:
  GPIO() = default;
  enum class Direction
  {
    OUT,
    IN
  };

  static bool exportGPIO(int gpioPin, bool unexport = false)
  {
    // Export GPIO pin
    std::ofstream export_file(unexport ? "/sys/class/gpio/unexport"
                                       : "/sys/class/gpio/export");
    if (!export_file.is_open()) {
      std::cerr << "Failed to open GPIO " << (unexport ? "unexport" : "export")
                << " file: " << strerror(errno) << std::endl;
      return false;
    }
    export_file << gpioPin;
    return true;
  }
  static bool setDirection(int gpioPin, Direction direction)
  {
    std::stringstream direction_path;
    direction_path << "/sys/class/gpio/gpio" << gpioPin << "/direction";
    std::ofstream direction_file(direction_path.str());
    if (!direction_file.is_open()) {
      std::cerr << "Failed to open GPIO direction file: " << strerror(errno)
                << std::endl;
      return false;
    }
    direction_file << (direction == Direction::IN ? "in" : "out");
    return true;
  }
  static std::ofstream openGPIOValueFile(int gpioPin)
  {
    // Open GPIO value file for writing
    std::stringstream value_path;
    value_path << "/sys/class/gpio/gpio" << gpioPin << "/value";
    std::ofstream value_file(value_path.str());
    if (!value_file.is_open()) {
      std::cerr << "Failed to open GPIO value file: " << strerror(errno)
                << std::endl;
      throw std::runtime_error("Error opening GPIO value file");
    }
    return value_file;
  }
  ~GPIO() = default;
};

int
main()
{
  constexpr int gpioPin = 55; /* Specify your GPIO pin here */
  if (!GPIO::exportGPIO(gpioPin)) {
    std::cerr << "Failed to export GPIO pin " << gpioPin << std::endl;
    return -1;
  }

  // Set GPIO direction to "out"
  if (!GPIO::setDirection(gpioPin, GPIO::Direction::OUT)) {
    std::cerr << "Failed to set direction on GPIO pin " << gpioPin << std::endl;
    return -1;
  }

  auto gpioValueFile = GPIO::openGPIOValueFile(gpioPin);
  while (true) {
    gpioValueFile << "1" << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    gpioValueFile << "0" << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  gpioValueFile.close();

  // Unexport GPIO pin
  GPIO::exportGPIO(gpioPin, true);

  return 0;
}
