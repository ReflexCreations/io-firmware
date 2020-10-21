#ifndef __COMMANDS_H
#define __COMMANDS_H;

typedef enum {
  CMD_REQUEST_SENSORS = 0x10,
  CMD_TRANSMIT_LED_DATA = 0x20,
  CMD_COMMIT_LEDS = 0x30
} Commands;

#endif