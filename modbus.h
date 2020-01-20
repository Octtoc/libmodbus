/*
 * modbus.h
 *
 *  Created on: 20.01.2020
 *      Author: Michael
 */

#ifndef MODBUS_H_
#define MODBUS_H_
#include <avr/io.h>

struct frame{
	uint8_t frameIndex;
	uint8_t frameMaxCounter;
	uint8_t frameField[128];
};
typedef struct frame frame_t;

struct mb_coil{
	uint8_t function_code;
	uint16_t starting_address;
	uint16_t quantity_coils;
	uint8_t byte_count;
	uint8_t coil_status[125];
	uint16_t crcValue;
};
typedef struct mb_coil mb_coil_t;

struct mb_holding_register{
	uint8_t function_code;
	uint16_t starting_address;
	uint16_t quantity_registers;
	uint8_t byte_count;
	uint8_t register_value[125];
	uint16_t crcValue;
};
typedef struct mb_holding_register mb_holding_register_t;

#endif /* MODBUS_H_ */
