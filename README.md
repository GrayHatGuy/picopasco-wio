# picopasco-wio
full port of picopasco mixing to WIO added onboard sensors for sound, light, and accelerometer.

## Instructions:
    - Add SGP30 and SHT4X I2C sensors otherwise it will fail startup - blank terminal
    - Setup Arduino per [Wio How To](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/)
    - Install libraries listed in #include 
    - Compile and upload
 
# OPEN ITEMS
      [] - Verify relay/pump control pins
      [] - device_type define in device.h for wio and pico sensor and pin differences
      [] - UI from example in [lv_wiopasco](https://github.com/GrayHatGuy/lv_wiopasco)
      [] - LoRAWAN networking


