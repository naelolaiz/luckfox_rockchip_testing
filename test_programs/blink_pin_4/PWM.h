#ifndef __PWM_H__
#define __PWM_H__

#include <cerrno>  // For errno
#include <cstdlib> // For system
#include <cstring> // For strerror
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread> // For std::this_thread::sleep_for

class PWM
{
public:
  PWM() = default;

  static bool exportPWM(int chipNr, int pwmNr, bool unexport = false)
  {
    std::ostringstream oss;
    oss << "/sys/class/pwm/pwmchip" << chipNr
        << (unexport ? "/unexport" : "/export");
    std::ofstream export_file(oss.str());
    if (!export_file.is_open()) {
      std::cerr << "Failed to open PWM " << (unexport ? "unexport" : "export")
                << " file: " << strerror(errno) << std::endl;
      return false;
    }
    export_file << pwmNr;
    return true;
  }

  static bool setPeriodInNs(int chipNr, int pwmNr, size_t period)
  {
    std::ostringstream oss;
    oss << "/sys/class/pwm/pwmchip" << chipNr << "/pwm" << pwmNr << "/period";
    std::ofstream period_file(oss.str());
    if (!period_file.is_open()) {
      std::cerr << "Failed to open PWM period file: " << strerror(errno)
                << std::endl;
      return false;
    }
    period_file << period;
    return true;
  }

  static bool setDutyCycleInNs(int chipNr, int pwmNr, size_t period)
  {
    std::ostringstream oss;
    oss << "/sys/class/pwm/pwmchip" << chipNr << "/pwm" << pwmNr
        << "/duty_cycle";
    std::ofstream duty_cycle_file(oss.str());
    if (!duty_cycle_file.is_open()) {
      std::cerr << "Failed to open PWM duty_cycle file: " << strerror(errno)
                << std::endl;
      return false;
    }
    duty_cycle_file << period;
    return true;
  }
  enum class Polarity
  {
    NORMAL,
    INVERSED
  };
  static bool setPolarity(int chipNr, int pwmNr, Polarity polarity)
  {
    std::ostringstream oss;
    oss << "/sys/class/pwm/pwmchip" << chipNr << "/pwm" << pwmNr << "/polarity";
    std::ofstream polarity_file(oss.str());
    if (!polarity_file.is_open()) {
      std::cerr << "Failed to open PWM polarity file: " << strerror(errno)
                << std::endl;
      return false;
    }
    polarity_file << (polarity == Polarity::NORMAL ? "normal" : "inversed");
    return true;
  }
  static bool setEnabled(int chipNr, int pwmNr, bool enabled)
  {
    std::ostringstream oss;
    oss << "/sys/class/pwm/pwmchip" << chipNr << "/pwm" << pwmNr << "/enable";
    std::ofstream enable_file(oss.str());
    if (!enable_file.is_open()) {
      std::cerr << "Failed to open PWM enable file: " << strerror(errno)
                << std::endl;
      return false;
    }
    enable_file << (enabled ? 1 : 0);
    return true;
  }

  ~PWM() = default;
};

#endif // __PWM_H__
