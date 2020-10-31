#include "color.h"

static double hue_to_rgb(double, double, double);

// This and hue_to_rgb taken from:
// https://stackoverflow.com/a/9493060
Color_RGB color_hsl_to_rgb(Color_HSL hsl) {
    Color_RGB rgb;

    // Range: 0...1
    double r, g, b;

    // Limit inputs to expected ranges
    hsl.hue = hsl.hue > 360 ? 360 : hsl.hue;
    hsl.saturation = hsl.saturation > 100 ? 100 : hsl.saturation;
    hsl.lightness = hsl.lightness > 100 ? 100 : hsl.lightness;

    // Re-range HSL to 0...1 doubles
    double h = (double)hsl.hue / 360.0;
    double s = (double)hsl.saturation / 100.0;
    double l = (double)hsl.lightness / 100.0;

    if (s == 0) {
        // Saturation == 0 means rgb values are identical, grey.
        rgb.red = rgb.green = rgb.blue = l * 255.0;
        return rgb;
    }

    double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    double p = 2.0 * l - q;

    r = hue_to_rgb(p, q, h + 1 / 3.0);
    g = hue_to_rgb(p, q, h);
    b = hue_to_rgb(p, q, h - 1 / 3.0);

    rgb.red = r * 255.0;
    rgb.green = g * 255.0;
    rgb.blue = b * 255.0;

    return rgb;
}

// Look, I don't have a clue how this works, okay?
static double hue_to_rgb(double p, double q, double t) {
    if (t < 0) t += 1.0;
    if (t > 1) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}