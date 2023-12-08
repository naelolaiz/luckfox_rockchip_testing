set(target_name hello_world)

add_executable(${target_name} main.cpp)

install(TARGETS ${target_name} DESTINATION bin)
