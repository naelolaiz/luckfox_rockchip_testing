# Lissajous projections with two servos and a LASER pointer LED

## servos
https://www.handsontec.com/dataspecs/motor_fan/MG996R.pdf

## servo cases
https://www.thingiverse.com/thing:3458238

## LED pointer
https://www.katranji.com/tocimages/files/336817-121153.pdf

## Circuit diagram
Notice that a common ground connection is missing from any of the SoC board ground pins (pin 3, 8, 13, 18, 38, 33, 28 or 23)
![Circuit diagram](doc/circuit_diagram.jpg)

## use examples
### "Hearth" 
./pwm_two_servos --updateStep 0.15 --servoYawFreq 1 --servoPitchFre
q 1 --servoPitchIP 0.25 --servoYawIP 0.0 --servoYawMaxValInNs 1000000 --servoYaw
MinValInNs 500000 --servoPitchMaxValInNs 1900000 --servoPitchMinValInNs 1300000 
   --enableLaser 4 --spiral 2

### Lisajjous
./pwm_two_servos --updateStep 0.15 --servoYawFreq 1 --servoPitchFre
q 2 --servoPitchIP 0.0 --servoYawIP 0.0 --servoYawMaxValInNs 1000000 --servoYawM
inValInNs 500000 --servoPitchMaxValInNs 1900000 --servoPitchMinValInNs 1300000  
  --enableLaser 4 --spiral 6

./pwm_two_servos --updateStep 0.035 --servoYawFreq 2 --servoPi
tchFreq 6 --servoPitchIP 0.0 --servoYawIP 0.25 --servoYawMaxValInNs 850000 --ser
voYawMinValInNs 450000 --servoPitchMaxValInNs 1800000 --servoPitchMinValInNs 150
0000    --enableLaser 4
![lissajous example](doc/lissajous.gif)

### infinity

./pwm_two_servos --updateStep 0.10 --servoYawFreq 1 --servoPitchFre
q 2 --servoPitchIP 0.0 --servoYawIP 0.25 --servoYawMaxValInNs 850000 --servoYawM
inValInNs 650000 --servoPitchMaxValInNs 1600000 --servoPitchMinValInNs 1400000
  --enableLaser 4 --spiral 20

### Hello world
--servoYawMaxValInNs 850000 --servoYawMinValInNs 450000 --ser
voPitchMaxValInNs 1700000 --servoPitchMinValInNs 1500000   --enableLaser 34 --wa
itBetweenPaths 4 --stride 2

![Hello world SVG](doc/long_exposure_hello_world.png)
