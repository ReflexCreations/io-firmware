#ifndef __COMTESTS_H
#define __COMTESTS_H

#include "stm32f3xx.h"
#include "msgbus.h"
#include "request.h"
#include "stdbool.h"

uint8_t commtest_receive_2bytes(ComportId);
uint8_t commtest_receive_64bytes(ComportId);
uint8_t commtest_double_values(ComportId);

uint8_t commtest_dual_receive_2bytes(ComportId, ComportId);
uint8_t commtest_dual_receive_64bytes(ComportId, ComportId);
uint8_t commtest_dual_double_values(ComportId, ComportId);

#endif