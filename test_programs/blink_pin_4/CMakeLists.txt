project(blink_pin_4)

add_executable(${PROJECT_NAME} main.cpp
    GPIO.h
    PWM.h)
target_include_directories(${PROJECT_NAME} PUBLIC .)
