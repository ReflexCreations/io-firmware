#include "ledtests.h"
#include "commands.h"
#include "request.h"
#include "msgbus.h"

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