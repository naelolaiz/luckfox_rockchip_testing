set(target_name peripherals_interface)

add_library(${target_name}
             INTERFACE
             GPIO.h
             PWM.h)

         target_include_directories(${target_name} INTERFACE .)
