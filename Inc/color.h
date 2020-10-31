#ifndef __COLOR_H
#define __COLOR_H

#include "stm32f3xx.h"

typedef struct {
    // Red in 0 to 255
    uint8_t red;

    // Green in 0 to 255
    uint8_t green;

    // Blue in 0 to 255
    uint8_t blue;
} Color_RGB;

typedef struct {
    // Hue in degrees, 0 to 360
    uint16_t hue;

    // Saturation in percent, 0 to 100
    uint8_t saturation;

    // Lightness in percent, 0 to 100
    uint8_t lightness;
} Color_HSL;

Color_RGB color_hsl_to_rgb(Color_HSL);

#endif