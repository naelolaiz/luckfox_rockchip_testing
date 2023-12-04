cmake_minimum_required(VERSION 3.22)
project(test_programs)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER
    /build_env/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc)
set(CMAKE_CXX_COMPILER
    /build_env/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++)

# where is the target environment located
# This expectes to use the container luckfoxtech/luckfox_pico:1.0 with mounted https://github.com/LuckfoxTECH/luckfox-pico.git. See https://wiki.luckfox.com/Luckfox-Pico/Core3566-SDK/
set(CMAKE_FIND_ROOT_PATH  /build_env/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/)

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

add_subdirectory(hello_world)
add_subdirectory(blink_pin_4)
