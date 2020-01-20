/*
 * modbus-functions.c
 *
 *  Created on: 20.01.2020
 *      Author: Michael
 */

#include "modbus-functions.h"
#include "modbus-rtu.h"

uint8_t MB_FillHoldingRegister(mb_function_holding_register *holding_register, frame *FReceiveFrame) {
	uint16_t u16StartingAddressLeftData = 0;

	//Activate Function in Main to process Data and receive Transmission Frame
	holding_register->quantity_registers = ((uint16_t) FReceiveFrame->frame[4] << 8) | FReceiveFrame->frame[5];
	holding_register->starting_address = ((uint16_t) FReceiveFrame->frame[2] << 8) | FReceiveFrame->frame[3];
	holding_register->byte_count = holding_register->quantity_registers * 2;

	//ExceptionCode Handling
	if (holding_register->quantity_registers > 0 && holding_register->quantity_registers <= 0x007D) {
		//MB_AddExceptionFrameToTransmitBuffer(ILLEGAL_DATA_VALUE, currentMB_State);
		return ILLEGAL_DATA_VALUE;
	}

	u16StartingAddressLeftData = 0xFFFF - holding_register->starting_address;
	if (u16StartingAddressLeftData > holding_register->quantity_registers) {
		//MB_AddExceptionFrameToTransmitBuffer(ILLEGAL_DATA_ADDRESS, currentMB_State);
		return ILLEGAL_DATA_ADDRESS;
	}
	//END Exception Handling

	MB_PORT_ResponseHoldingRegisters(holding_register);
	holding_register->crcValue = usMBCRC16(TransmitFrame, 3 + holding_register->byte_count);

	return 0;
}

uint8_t MB_AddHoldingRegisterToFrame(mb_function_holding_register *holding_register, frame *FTransmitFrame) {
	//ADD Data to Transmit Frame Buffer
	uint8_t i = 0;
	uint16_t u16CRC;

	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = MB_SLAVE_ADDRESS;
	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = READ_HOLDING_REGISTER;
	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = holding_register->byte_count;

	i = 0;
	while (i < holding_register->byte_count) {
		FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = holding_register->register_value[i];
		i++;
	}
	u16CRC = usMBCRC16(FTransmitFrame->frame, 3 + holding_register->byte_count);
	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = u16CRC & 0x00FF;
	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = (u16CRC & 0xFF00) >> 8;

	return 0;
}

uint8_t MB_FillReadCoil(mb_function_coil *function_coil, frame *FReceiveFrame) {
	function_coil->starting_address = (FReceiveFrame->frame[2] << 8) | FReceiveFrame->frame[3];
	function_coil->quantity_coils = (FReceiveFrame->frame[4] << 8) | FReceiveFrame->frame[5];
	function_coil->byte_count = function_coil->quantity_coils / 8;

	if ((function_coil->byte_count % 8) > 1) {
		function_coil->byte_count++;
	}

	MB_PORT_SendReadCoils(function_coil);
	return 0;
}

uint8_t MB_AddReadCoilToFrame(mb_function_coil *function_coil, frame *FTransmitFrame) {
	//ADD Data to Transmit Frame Buffer
	uint8_t i = 0;
	uint16_t u16CRC;

	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = MB_SLAVE_ADDRESS;
	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = READ_HOLDING_REGISTER;
	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = function_coil->byte_count;

	i = 0;
	while (i < function_coil->byte_count) {
		FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = function_coil->coil_status[i];
		i++;
	}

	u16CRC = usMBCRC16(FTransmitFrame->frame, 3 + function_coil->byte_count);
	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = u16CRC & 0x00FF;
	FTransmitFrame->frame[FTransmitFrame->frameMaxCounter++] = (u16CRC & 0xFF00) >> 8;

	return 0;
}
