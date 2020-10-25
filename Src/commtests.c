#include "commtests.h"
#include "debug_leds.h"

static Response * wait_for_response();

static uint8_t verify_data_2bytes(uint8_t *);
static uint8_t verify_data_64bytes(uint8_t *);
static uint8_t verify_data_double(uint8_t *, uint8_t *);

uint8_t commtest_receive_2bytes(ComportId comport_id) {
    uint8_t rec_data[2];

    rec_data[0] = 0x00;
    rec_data[1] = 0x00;

    Request req;
    req.comport_id = comport_id;
    req.request_command = Command_Test_Expect_2B;
    req.send_data = NULL;
    req.send_data_len = 0;
    req.response_data = rec_data;
    req.response_len = 2;

    msgbus_send_request(req);
    DBG_LED2_ON();

    Response * resp = wait_for_response();

    if (resp->request_command != Command_Test_Expect_2B) return false;
    return verify_data_2bytes(rec_data);
}

uint8_t commtest_dual_receive_2bytes(ComportId port1, ComportId port2) {
    uint8_t rec_data[4];

    Request req;
    req.comport_id = port1;
    req.request_command = Command_Test_Expect_2B;
    req.send_data = NULL;
    req.send_data_len = 0;
    req.response_data = rec_data;
    req.response_len = 2;

    msgbus_send_request(req);

    req.comport_id = port2;
    req.response_data = rec_data + 2;

    msgbus_send_request(req);

    uint8_t responses = 0;

    while (responses != 0B11) {
        Response * resp = wait_for_response();

        if (resp->request_command != Command_Test_Expect_2B) return false;

        // Check if response is in fact for one of our ports
        if (resp->comport_id != port1 && resp->comport_id != port2) {
            return false;
        }

        uint8_t offset = resp->comport_id == port1 ? 0 : 2;

        if (verify_data_2bytes(rec_data + offset)) {
            responses |= resp->comport_id == port1 ? 0B01 : 0B10;
        }
    }

    return true;
}

uint8_t commtest_receive_64bytes(ComportId comport_id) {
    uint8_t rec_data[2 * 2];

    Request req;
    req.comport_id = comport_id;
    req.request_command = Command_Test_Expect_64B;
    req.send_data = NULL;
    req.send_data_len = 0;
    req.response_data = rec_data;
    req.response_len = 64;

    msgbus_send_request(req);

    Response * resp = wait_for_response();

    if (resp->request_command != Command_Test_Expect_64B) return false;

    return verify_data_64bytes(rec_data);
}

uint8_t commtest_dual_receive_64bytes(ComportId port1, ComportId port2) {
    uint8_t data_length = 64;
    uint8_t rec_data[data_length * 2];

    Request req;
    req.comport_id = port1;
    req.request_command = Command_Test_Expect_64B;
    req.send_data = NULL;
    req.send_data_len = 0;
    req.response_data = rec_data;
    req.response_len = data_length;

    msgbus_send_request(req);

    req.comport_id = port2;
    req.response_data = rec_data + data_length;

    msgbus_send_request(req);

    uint8_t responses = 0;

    while (responses != 0B11) {
        Response * resp = wait_for_response();

        if (resp->request_command != Command_Test_Expect_64B) return false;

        // Check if response is in fact for one of our ports
        if (resp->comport_id != port1 && resp->comport_id != port2) {
            return false;
        }

        uint8_t offset = resp->comport_id == port1 ? 0 : data_length;

        if (verify_data_64bytes(rec_data + offset)) {
            responses |= resp->comport_id == port1 ? 0B01 : 0B10;
        }
    }

    return true;
}

uint8_t commtest_double_values(ComportId comport_id) {
    uint8_t send_data[64];
    uint8_t rec_data[64];

    Request req;
    req.comport_id = comport_id;
    req.request_command = Command_Test_Double_Values;
    req.send_data = send_data;
    req.send_data_len = 64;
    req.response_data = rec_data;
    req.response_len = 64;

    for (uint8_t i =  0; i < 64; i++) {
        send_data[i] = i;
    }

    msgbus_send_request(req);

    Response * resp = wait_for_response();

    if (resp->request_command != Command_Test_Double_Values) return false;

    return verify_data_double(send_data, rec_data);
}

uint8_t commtest_dual_double_values(ComportId port1, ComportId port2) {
    uint8_t data_length = 64;
    uint8_t rec_data[data_length * 2];
    uint8_t send_data[data_length];

    for (uint8_t i = 0; i < data_length; i++) {
        send_data[i] = i;
    }

    // NOTE: possibly we'd want send_data to be double length too?

    Request req;
    req.comport_id = port1;
    req.request_command = Command_Test_Double_Values;
    req.send_data = send_data;
    req.send_data_len = data_length;
    req.response_data = rec_data;
    req.response_len = data_length;

    msgbus_send_request(req);

    req.comport_id = port2;
    req.response_data = rec_data + data_length;

    msgbus_send_request(req);

    uint8_t responses = 0;

    while (responses != 0B11) {
        Response * resp = wait_for_response();

        if (resp->request_command != Command_Test_Double_Values) return false;

        // Check if response is in fact for one of our ports
        if (resp->comport_id != port1 && resp->comport_id != port2) {
            return false;
        }

        uint8_t offset = resp->comport_id == port1 ? 0 : data_length;

        if (verify_data_double(send_data, rec_data + offset)) {
            responses |= resp->comport_id == port1 ? 0B01 : 0B10;
        }
    }

    return true;
}

static uint8_t verify_data_2bytes(uint8_t * data) {
    return data[0] == 0xBE && data[1] == 0xEF;
}

static uint8_t verify_data_64bytes(uint8_t * data) {
    for (uint8_t i = 0; i < 64; i++) {
        if (data[i] != i + 1) return false;
    }

    return true;
}

static uint8_t verify_data_double(uint8_t * req_data, uint8_t * resp_data) {
    for (uint8_t i = 0; i < 64; i++) {
        if (resp_data[i] != req_data[i] * 2) return false;
    }

    return true;
}

static Response * wait_for_response() {
    while(!msgbus_have_pending_response()) {
        msgbus_process_flags();
        msgbus_switch_ports_if_done();
    }

    return msgbus_get_pending_response();
}
