set(target_name blink_gpio_pin4)

add_executable(${target_name} main.cpp)
target_link_libraries(${target_name} PUBLIC peripherals_interface)
