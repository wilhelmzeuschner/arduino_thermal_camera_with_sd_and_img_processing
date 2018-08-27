# Thermal Camera with image saving capability
Original programm and 3D-printed case are from: https://www.thingiverse.com/thing:2799023
An explanation of what this code does and what to look out for can be found on the Thingiverse page.
I run my ESP8266 at 160MHz -> higher framerate (select in Arduino Board Options).

To connect the SD card, you have to share some of the SPI pins with the TFT (MOSI; SCK; [MISO])
SD Slave Select is defined in the code, aswell as the button pin (which can't be D0 since that pin does not support interrupts)

You can choose whether you want to save an 8x8 image or the interpolated 70x70 image.

Sorry for the rather messy python code. It works.
I got very useful help from: https://stackoverflow.com/questions/52027382/how-do-i-convert-a-csv-file-16bit-high-color-to-image-in-python

Some processed images:


![](https://github.com/wilhelmzeuschner/arduino_thermal_camera_with_sd_and_img_processor/blob/master/images/img.png)
![](https://github.com/wilhelmzeuschner/arduino_thermal_camera_with_sd_and_img_processor/blob/master/images/thermal_image.png)
