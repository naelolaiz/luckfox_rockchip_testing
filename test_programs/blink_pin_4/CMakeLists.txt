set(target_name blink_pin_4)

add_executable(${target_name} main.cpp
    GPIO.h
    PWM.h)

target_include_directories(${target_name} PUBLIC .)
