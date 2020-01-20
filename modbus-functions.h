/*
 * modbus-functions.h
 *
 *  Created on: 20.01.2020
 *      Author: Michael
 */

#ifndef MODBUS_FUNCTIONS_H_
#define MODBUS_FUNCTIONS_H_

#include <avr/io.h>

typedef struct {
	uint8_t function_code;
	uint16_t starting_address;
	uint16_t quantity_coils;
	uint8_t byte_count;
	uint8_t coil_status[125];
	uint16_t crcValue;
} mb_function_coil;

typedef struct {
	uint8_t function_code;
	uint16_t starting_address;
	uint16_t quantity_registers;
	uint8_t byte_count;
	uint8_t register_value[125];
	uint16_t crcValue;
} mb_function_holding_register;

uint8_t MB_FillHoldingRegister(mb_function_holding_register *holding_register, uint8_t *u8pReceiveFrame);
uint8_t MB_AddHoldingRegisterToFrame(mb_function_holding_register *holding_register, uint8_t *u8pTransmitFrame);

uint8_t MB_FillReadCoil(mb_function_coil *function_coil, uint8_t *u8pReceiveFrame);
uint8_t MB_AddReadCoilToFrame(mb_function_coil *function_coil, uint8_t *u8pTransmitFrame);

#endif /* MODBUS_FUNCTIONS_H_ */
