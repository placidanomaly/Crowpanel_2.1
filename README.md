# Crowpanel_2.1

An Arduino library to use the Crowpanel 2.1" rotary controller and LCD display like this one: https://www.elecrow.com/crowpanel-2-1inch-hmi-esp32-rotary-display-480-480-ips-round-touch-knob-screen.html .

<img src="https://media-cdn.elecrow.com/media/webp_image/catalog/product/cache/1917af0aa5ac872287906ce8b861b1aa/2/_/2.1inch_rotary_knob_screen_1.webp" alt="Crowpanel 2.1 picture" width=300>

# Introduction
The library API is documented within the [crowpanel_2_1.h](https://github.com/placidanomaly/Crowpanel_2.1/blob/main/src/crowpanel_2_1.h) header file. An [example Arduino file](https://github.com/placidanomaly/Crowpanel_2.1/blob/main/examples/crowpanel_2_1_demo/crowpanel_2_1_demo.ino) shows how to use most of its functionality.

# Summary of Features

The following capabilities are provided:

- Full initialization of all components
- LCD power ON/OFF and reset
- Display rotation
- Backlight intensity
- Touchscreen event handling through the LVGL input device framework
- Rotary control event handling including clicks, double-clicks, long presses, and rotation

There are no special helper functions needed to access the UART or I2C pins on the back of the device, this can be done with the standard Arduino libraries.

# Arduino IDE settings

In order to compile this library and the example program successfully, the following settings must be applied in the Arduino IDE:

- Board type: ESP32S3 Dev Module (add this library if needed: https://espressif.github.io/arduino-esp32/package_esp32_index.json )

- USB CDC On Boot: Enabled
- Flash size: 16MB
- PSRAM: OPI PSRAM
- Partition Scheme: Huge App

# Example Programs

## very_simple

This is a minimalistic program that can be used as a skeleton to use the library in a project. Just open the [very_simple.ino](https://github.com/placidanomaly/Crowpanel_2.1/blob/main/examples/very_simple/very_simple.ino) file in Arduino IDE, ensure that the settings in the previous section have been selected, and hit the Upload button. The program displays a text label in the center of the display and outputs a message on the serial port whenever the rotary encoder is clicked (use the Serial Monitor tool in Arduino IDE to see the message).

## crowpanel_2_1_demo

This is a more substantial example that exercises almost all of the capabilities of the library. The [example program](https://github.com/placidanomaly/Crowpanel_2.1/blob/main/examples/crowpanel_2_1_demo/crowpanel_2_1_demo.ino) displays a curved slider to control the brightness of the LCD display of the Crowpanel 2.1".

A desired target brightness is set by clicking on the slider. The target brightness marker
will move to the selected target value and the brightness of the LCD will gradually change to match the target value. The actual brightness of the display is indicated by a solid circle while the target marker is indicated by a hollow circle.

The target marker can also be dragged with normal touchscreen drag gestures, demonstrating the use of LVGL object dragging capabilities and input device callbacks to process touchscreen signals.

The target brightness can also be changed by rotating the encoder. A clockwise rotation increases brightness while a counterclockwise will decrease it. This demonstrates the use of the rotary encoder.

The brightness can be reset to its initial value by holding the encoder down for 1 second or more, demonstrating one of the 3 possible click callbacks from the encoder.

# Future Improvements

It is unclear whether this library would work with other Crowpanel models. It would be good to have a single library offering support for all devices.

# Contact Information

For more information, bug reports, etc., contact [Charles Vaillancourt](mailto:charles.vaillancourt).
