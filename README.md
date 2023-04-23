# picopasco-wio
port of [picopasco](https://github.com/GrayHatGuy/picopasco) for WIO added onboard sensors for sound, light, and accelerometer. reads 11 sensors and controls 4+ relays.

## Instructions:

- [parts demo - ](https://youtu.be/lR3D1bXBkDE)
- add SGP30 and SHT4X I2C sensors to J5 (power switch side) otherwise it will fail startup - blank terminal
- Connect GPIO breakout.  
- Connect jumper wires to pump relay and WIO breakout. (wires are color coded)
- setup Arduino per [Wio How To](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/)
- install libraries listed in #include 
- compile and upload
- view terminal [output](https://youtu.be/uyEobShH6yY)

### [functionsl prototype demo](https://youtu.be/b17KJY9SBbU)

# OPEN ITEMS

* universal code for mcu device type header
* UI from example in [lv_wiopasco](https://github.com/GrayHatGuy/lv_wiopasco)
* LoRAWAN networking
+ Add pinout table
+ GPIO breakout
