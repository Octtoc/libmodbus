/*
 * modbus-functions.h
 *
 *  Created on: 20.01.2020
 *      Author: Michael
 */

#ifndef MODBUS_FUNCTIONS_H_
#define MODBUS_FUNCTIONS_H_

#include <avr/io.h>
#include "modbus-rtu.h"

uint8_t MB_FillHoldingRegister(mb_holding_register_t *holding_register, frame_t *FReceiveFrame);
uint8_t MB_AddHoldingRegisterToFrame(mb_holding_register_t *holding_register, frame_t *FTransmitFrame);

uint8_t MB_FillReadCoil(mb_coil_t *function_coil, frame_t *FReceiveFrame);
uint8_t MB_AddReadCoilToFrame(mb_coil_t *function_coil, frame_t *FTransmitFrame);

#endif /* MODBUS_FUNCTIONS_H_ */
