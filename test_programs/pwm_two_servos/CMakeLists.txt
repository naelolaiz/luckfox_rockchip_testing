set(target_name pwm_two_servos)

add_executable(${target_name} main.cpp svg/nael.cpp svg/nael.h)
target_link_libraries(${target_name} PRIVATE peripherals_interface)
target_include_directories(${target_name} PRIVATE .)

install(TARGETS ${target_name} DESTINATION bin)
install(FILES S92pwm_two_servos DESTINATION etc/init.d)


