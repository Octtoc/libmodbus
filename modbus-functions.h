/*
 * modbus-function.h
 *
 *  Created on: 22.01.2020
 *      Author: Michael
 */

#ifndef MODBUS_FUNCTIONS_H_
#define MODBUS_FUNCTIONS_H_

#include "modbus.h"

uint8_t MB_AddExceptionToFrame(mb_exception_t *exception,
		frame_t *FTransmitFrame);

uint8_t MB_FillHoldingRegister(mb_holding_register_t *holding_register,
		frame_t *FReceiveFrame);
uint8_t MB_AddHoldingRegisterToFrame(mb_holding_register_t *holding_register,
		frame_t *FTransmitFrame);

uint8_t MB_FillReadCoil(mb_coil_t *function_coil, frame_t *FReceiveFrame);
uint8_t MB_AddReadCoilToFrame(mb_coil_t *function_coil, frame_t *FTransmitFrame);

uint8_t MB_FillWriteCoil(mb_write_coil_t *function_write_coil,
		frame_t *FReceiveFrame);
uint8_t MB_AddWriteCoilToFrame(mb_write_coil_t *function_write_coil,
		frame_t *FTransmitFrame);

#endif /* MODBUS_FUNCTIONS_H_ */
