#ifndef __GPIO_H__
#define __GPIO_H__

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

#endif // __GPIO_H__
