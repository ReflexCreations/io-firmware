#ifndef __LEDTESTS_H
#define __LEDTESTS_H

#include "uart.h"

void ledtests_hardcoded_LEDs(ComportId);

void ledtests_solid_color_LEDs(ComportId, uint8_t, uint8_t, uint8_t);

void ledtests_segments_solid_color(
    ComportId, uint8_t, uint8_t, uint8_t, uint8_t);

void ledtests_commit_LEDs(ComportId);

void ledtests_loop_color_wheel(ComportId);

#endif