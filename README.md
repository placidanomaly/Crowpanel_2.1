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

# Future Improvements

It is unclear whether this library would work with other Crowpanel models. It would be good to have a single library offering support for all devices.

# Contact Information

For more information, bug reports, etc., contact [Charles Vaillancourt](mailto:charles.vaillancourt).
