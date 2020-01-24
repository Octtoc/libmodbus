/*
 * modbus-functions.c
 *
 *  Created on: 20.01.2020
 *      Author: Michael
 */

#include "modbus-functions.h"
#include "modbus-rtu.h"

uint8_t MB_AddExceptionToFrame(mb_exception_t *exception,
		frame_t *FTransmitFrame) {
	uint16_t u16CRC = 0;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
	MB_SLAVE_ADDRESS;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			exception->functionCode + 0x80;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			exception->exceptionCode;

	u16CRC = usMBCRC16(FTransmitFrame->frameField, 3);
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] = u16CRC
			& 0x00FF;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] = (u16CRC
			& 0xFF00) >> 8;
	return 0;
}

uint8_t MB_AddHoldingRegisterToFrame(mb_holding_register_t *holding_register,
		frame_t *FTransmitFrame) {
	//ADD Data to Transmit Frame Buffer
	uint8_t i = 0;
	uint16_t u16CRC;

	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
	MB_SLAVE_ADDRESS;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			READ_HOLDING_REGISTER;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			holding_register->byte_count;

	i = 0;
	while (i < holding_register->byte_count) {
		FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
				holding_register->register_value[i];
		i++;
	}
	u16CRC = usMBCRC16(FTransmitFrame->frameField,
			3 + holding_register->byte_count);
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] = u16CRC
			& 0x00FF;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] = (u16CRC
			& 0xFF00) >> 8;

	return 0;
}

uint8_t MB_FillReadCoil(mb_coil_t *function_coil, frame_t *FReceiveFrame) {
	function_coil->starting_address = (FReceiveFrame->frameField[2] << 8)
			| FReceiveFrame->frameField[3];
	function_coil->quantity_coils = (FReceiveFrame->frameField[4] << 8)
			| FReceiveFrame->frameField[5];
	function_coil->byte_count = function_coil->quantity_coils / 8;

	if ((function_coil->byte_count % 8) > 1) {
		function_coil->byte_count++;
	}

	MB_PORT_SendReadCoils(function_coil);
	return 0;
}

uint8_t MB_AddReadCoilToFrame(mb_coil_t *function_coil, frame_t *FTransmitFrame) {
	//ADD Data to Transmit Frame Buffer
	uint8_t i = 0;
	uint16_t u16CRC;

	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
	MB_SLAVE_ADDRESS;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] = READ_COILS;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			function_coil->byte_count;

	i = 0;
	while (i < function_coil->byte_count) {
		FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
				function_coil->coil_status[i];
		i++;
	}

	u16CRC = usMBCRC16(FTransmitFrame->frameField,
			FTransmitFrame->frameMaxCounter);
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] = u16CRC
			& 0x00FF;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] = (u16CRC
			& 0xFF00) >> 8;

	return 0;
}

uint8_t MB_FillHoldingRegister(mb_holding_register_t *holding_register,
		frame_t *FReceiveFrame) {
	uint16_t u16StartingAddressLeftData = 0;
	//Activate Function in Main to process Data and receive Transmission Frame
	holding_register->quantity_registers =
			((uint16_t) FReceiveFrame->frameField[4] << 8)
					| FReceiveFrame->frameField[5];
	holding_register->starting_address =
			((uint16_t) FReceiveFrame->frameField[2] << 8)
					| FReceiveFrame->frameField[3];
	holding_register->byte_count = holding_register->quantity_registers * 2;

	//ExceptionCode Handling
	if (holding_register->starting_address > 0
			&& holding_register->starting_address <= 0x007D) {
		//MB_AddExceptionFrameToTransmitBuffer(ILLEGAL_DATA_VALUE, currentMB_State);
		return ILLEGAL_DATA_VALUE;
	}
	u16StartingAddressLeftData = 0xFFFF - holding_register->starting_address;
	if (u16StartingAddressLeftData
			> (holding_register->quantity_registers * 2)) {
		//MB_AddExceptionFrameToTransmitBuffer(ILLEGAL_DATA_ADDRESS, currentMB_State);
		return ILLEGAL_DATA_ADDRESS;
	}
	//END Exception Handling
	MB_PORT_ResponseHoldingRegisters(holding_register);

	return 0;
}

uint8_t MB_FillWriteCoil(mb_write_coil_t *function_write_coil,
		frame_t *FReceiveFrame) {
	function_write_coil->outputAddress =
			((uint16_t) FReceiveFrame->frameField[2] << 8)
					| FReceiveFrame->frameField[3];
	function_write_coil->outputValue = ((uint16_t) FReceiveFrame->frameField[4]
			<< 8) | FReceiveFrame->frameField[5];
	return 0;
}

uint8_t MB_AddWriteCoilToFrame(mb_write_coil_t *function_write_coil,
		frame_t *FTransmitFrame) {
	uint16_t u16CRC = 0;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
	MB_SLAVE_ADDRESS;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			WRITE_SINGLE_COIL;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			function_write_coil->outputAddress >> 8;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			function_write_coil->outputAddress;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			function_write_coil->outputValue >> 8;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] =
			function_write_coil->outputValue;

	MB_PORT_WriteSingleCoil(function_write_coil);

	u16CRC = usMBCRC16(FTransmitFrame->frameField,
			FTransmitFrame->frameMaxCounter);
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] = u16CRC
			& 0x00FF;
	FTransmitFrame->frameField[FTransmitFrame->frameMaxCounter++] = (u16CRC
			& 0xFF00) >> 8;

	return 0;
}
