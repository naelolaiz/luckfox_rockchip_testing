# Testing Luckfox Pico RISC-V boards.

![Pico board](doc/Luckfox-Pico-1-1100x1100w.jpg)

* [WIKI](https://wiki.luckfox.com/Luckfox-Pico/Luckfox-Pico-quick-start/)
* [SDK](https://github.com/LuckfoxTECH/luckfox-pico)
* [Luckfox-Pico series schematic diagram](https://files.luckfox.com/wiki/Luckfox-Pico/PDF/SCH/Luckfox%20Pico%20%E5%8E%9F%E7%90%86%E5%9B%BE.7z)
* [RV1106 Datasheet](https://files.luckfox.com/wiki/Luckfox-Pico/PDF/Rockchip%20RV1106%20Datasheet%20V1.3-20230522.pdf)
* [RV1103 Datasheet](https://files.luckfox.com/wiki/Luckfox-Pico/PDF/doc.7z)
* [Extra info](https://wiki.luckfox.com/Luckfox-Pico/Datasheets/)

## Cross-compiling with cmake
Following instructions in [https://wiki.luckfox.com/Luckfox-Pico/Core3566-SDK/](https://wiki.luckfox.com/Luckfox-Pico/Core3566-SDK/), using docker image ```luckfoxtech/luckfox_pico:1.0``` with the [SDK](https://github.com/LuckfoxTECH/luckfox-pico) mounted in the container in ```/home```.

![Cross-compiling and running a hello world application](doc/cross_compiling_and_running_hello_world.png)
### Tests programs
See examples in [test_programs folder](test_programs/).

Compiled binaries published in [gitlab actions](https://github.com/naelolaiz/luckfox_rockchip_testing/actions/) jobs.

## RV1103
### PWM
#### Resolution
Even if the unit is a nanosecond, apparently the minimum resolution of the provided firmware kernel (Linux luckfox 5.10.110) is 10ns: 
![testing PWM resolution on RV1103](doc/testing_pwm_resolution_on_RV1103.png)

#### Jitter
It seems to have a jitter of ~5-10ns:
![testing PWM jitter on RV1103](doc/testing_pwm_jitter_on_RV1103.gif)

#### Stability
There are gaps up to ~70us on the PWM output when using small duty cycles (~10ns):
![testing PWM stability on RV1103](doc/testing_pwm_stability_on_RV1103.png)

This does not seem to happen when increasing the duty cycle to >20ns.
