set(target_name pwm_two_servos)

add_executable(${target_name} main.cpp)
set_target_properties(${target_name} PROPERTIES CXX_STANDARD 17)
target_link_libraries(${target_name} PUBLIC peripherals_interface)

install(TARGETS ${target_name} DESTINATION bin)
