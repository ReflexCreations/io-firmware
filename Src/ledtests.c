#include "ledtests.h"
#include "commands.h"
#include "request.h"
#include "msgbus.h"
#include "color.h"

static uint8_t input_data[64];

void ledtests_hardcoded_LEDs(ComportId port) {
    Request req;

    req.comport_id = port;
    req.request_command = Command_Test_Hardcoded_LEDs;
    req.send_data = NULL;
    req.send_data_len = 0;
    req.response_data = NULL;
    req.response_len = 0;

    msgbus_send_request(req);
}

void ledtests_solid_color_LEDs(
    ComportId port, uint8_t r, uint8_t g, uint8_t b) {

    input_data[0] = r;
    input_data[1] = g;
    input_data[2] = b;
    
    Request req;

    req.comport_id = port;
    req.request_command = Command_Test_Solid_Color_LEDs;
    req.send_data = input_data;
    req.send_data_len = 3;
    req.response_data = NULL;
    req.response_len = 0;

    msgbus_send_request(req);
}

void ledtests_segments_solid_color(
    ComportId port, uint8_t segment, uint8_t r, uint8_t g, uint8_t b) {

    uint8_t * storage = &input_data[segment * 4];

    storage[0] = segment & 0x03;
    storage[1] = r;
    storage[2] = g;
    storage[3] = b;

    Request req;

    req.comport_id = port;
    req.request_command = Command_Test_Segment_Solid_Color_LEDs;
    req.send_data = storage;
    req.send_data_len = 4;
    req.response_data = NULL;
    req.response_len = 0;

    msgbus_send_request(req);
}

void ledtests_commit_LEDs(ComportId port) {
    Request req;

    req.comport_id = port;
    req.request_command = Command_Test_Commit_LEDs;
    req.send_data = NULL;
    req.send_data_len = 0;
    req.response_data = NULL;
    req.response_len = 0;

    msgbus_send_request(req);
}

// An endless loop that animates a color wheel on a panel's LEDs
void ledtests_loop_color_wheel(ComportId port) {
    uint16_t base_hue = 0;
    uint16_t hue_step = 1;
    uint16_t hue_segment_offset = 30;

    while (1) {
        for (uint8_t seg = 0; seg < 4; seg++) {
            uint16_t seg_hue = (base_hue + seg * hue_segment_offset) % 360;
            Color_HSL hsl;
            hsl.hue = seg_hue;
            hsl.saturation = 100;
            hsl.lightness = 1;

            Color_RGB rgb = color_hsl_to_rgb(hsl);

            ledtests_segments_solid_color(
                Comport_Right, seg, rgb.red, rgb.green, rgb.blue
            );
        }

        ledtests_commit_LEDs(Comport_Right);
        msgbus_wait_for_idle(Comport_Right);

        HAL_Delay(10);
        
        base_hue = (base_hue + hue_step) % 360;
    }
}