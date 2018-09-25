# Thermal Camera with image saving capability
Original program and 3D-printed case are from: https://www.thingiverse.com/thing:2799023

An explanation of what this code does and what to look out for can be found on the Thingiverse page.
I run my ESP8266 at 160MHz -> higher framerate (select in Arduino Board Options).

To connect the SD card, you have to share some of the SPI pins with the TFT (MOSI; SCK; [MISO])

SD Slave Select is defined in the code, aswell as the button pin* (which can't be D0 since that pin does not support interrupts).
Schematic: https://easyeda.com/Manuauto/arduino_thermal_camera_with_sd_and_img_processing
*make sure to add a pull-up resistor

You can choose whether you want to save an 8x8 image or the interpolated 70x70 image.

Sorry for the rather messy python code. It works.
I got very useful help from: https://stackoverflow.com/questions/52027382/how-do-i-convert-a-csv-file-16bit-high-color-to-image-in-python

*It's quite useful to save the Python file to the directory you want the processed images to be saved to. Additionally, you might want to adjust the default "open file directory", so that it matches the drive letter of your SD Card.*

If you experience problems with your thermal camera rebooting or not even starting up properly (and are sure that it should work, i.e. it works over USB,) add more power supply decoupling! Use multiple small ceramic capacitors and maybe one larger electrolytic capacitor. If that does not help, use another method (step-up / step-down) regulator to supply 5 / 3.3V to the ESP8266. By supplying 5V you sacrifice a little bit of efficiency but you can usually gain a little bit of power supply stability since the 5V is regulated down to 3.3V with a linear regulator on the ESP8266 board. Even if you should't notice any problems immediately, it is gernerally a good idea to add sufficient decoupling. Make sure that the device can enter the OTA-mode without crashing.

**Some processed images:**


![](https://github.com/wilhelmzeuschner/arduino_thermal_camera_with_sd_and_img_processor/blob/master/images/thermal_image.png)
![](https://github.com/wilhelmzeuschner/arduino_thermal_camera_with_sd_and_img_processing/blob/master/images/thermal_image2.png)
![](https://github.com/wilhelmzeuschner/arduino_thermal_camera_with_sd_and_img_processor/blob/master/images/img.png)
![](https://github.com/wilhelmzeuschner/arduino_thermal_camera_with_sd_and_img_processing/blob/master/images/IMAGE_05.png)
![](https://github.com/wilhelmzeuschner/arduino_thermal_camera_with_sd_and_img_processing/blob/master/images/IMAGE_06.png)
![](https://github.com/wilhelmzeuschner/arduino_thermal_camera_with_sd_and_img_processing/blob/master/images/IMAGE_07.png)
