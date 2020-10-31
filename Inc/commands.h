#ifndef __COMMANDS_H
#define __COMMANDS_H

typedef enum {
  Command_None = 0x00,
  Command_Request_Sensors = 0x01,
  Command_Process_LED_Segment = 0x02,
  Command_Commit_LEDs = 0x03,

  Command_Test_Expect_2B = 0x71,
  Command_Test_Expect_64B = 0x72,
  Command_Test_Double_Values = 0x73,

  Command_Test_Hardcoded_LEDs = 0x81,
  Command_Test_Solid_Color_LEDs = 0x82,
  Command_Test_Segment_Solid_Color_LEDs = 0x83,
  Command_Test_Commit_LEDs = 0x84
} Commands;

#endif