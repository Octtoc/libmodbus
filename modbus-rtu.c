/*
 * modbus_rtu.c
 *
 * Created: 13.09.2019 20:37:43
 *  Author: Michael
 */ 

#include "modbus-rtu.h"

void MB_RTUInit() {
	modbus_state = MB_IDLE;
	modbus_response_time = 0;
	modbus_timer_1_5_is_expired = 0;
	modbus_timer_3_5_is_expired = 0;
	u8TransmitSizeIndex = 0;
	u8ReceiveSizeIndex = 0;
	
	RS485_TX_RX_SWITCH_DDR |= (1<<RS485_TX_RX_SWITCH_PIN);
	RS485_TX_RX_SWITCH_PORT &= ~(1<<RS485_TX_RX_SWITCH_PIN);
}

uint8_t MB_RequestHoldingRegisters(uint8_t _u8DeviceAdress,
					 uint16_t _u16StartingAddress, uint16_t _u16QuantityRegister) {
	if(modbus_state != MB_IDLE) {
		return modbus_state;
	}
	
	if (_u8DeviceAdress == 0x00 || _u8DeviceAdress >= 247) {
		//No valid slave Adress
		return 0;
	}
	
	uint8_t u8Function_Code = 0x03;
	uint8_t u8StartingAddressLow = _u16StartingAddress & 0x00FF;
	uint8_t u8StartingAddressHigh = _u16StartingAddress >> 8;
	uint8_t u8QuantityRegisterLow = _u16QuantityRegister & 0x00FF;
	uint8_t u8QuantityRegisterHigh = _u16QuantityRegister >> 8;
	uint16_t u16Crc;
	uint8_t u8Frame[20];
	
	u8Frame[0] = _u8DeviceAdress;
	u8Frame[1] = u8Function_Code;
	u8Frame[2] = u8StartingAddressHigh;
	u8Frame[3] = u8StartingAddressLow;
	u8Frame[4] = u8QuantityRegisterHigh;
	u8Frame[5] = u8QuantityRegisterLow;
	
	u16Crc = usMBCRC16(u8Frame, 6);
	
	u8Frame[6] = u16Crc & 0x00FF;
	u8Frame[7] = u16Crc >> 8;
	
	for(uint8_t u8iCountTransmit = 0; u8iCountTransmit < 8; u8iCountTransmit++) {
		//MB_TransmitByteFIFO(u8Frame[u8iCountTransmit]);
	}
	
	MB_START_TRANSMITTER;
	modbus_state = MB_Transmit;
	modbus_response_time = 0;
	
	return 1;
}

void MB_Turnaround() {
	if (MB_TimerT35Expired()) {
		MB_T35Reset();
		modbus_state = MB_IDLE;
	}
}

void MB_Transmit() {
	//If i can send data
	static uint8_t u8CurrentTransmissionFrame = 0;
	if (u8CurrentTransmissionFrame < u8TransmitSizeIndex) {
		if (MB_PORT_Transmit_Byte(
				u8pTransmitFrame[u8CurrentTransmissionFrame])) {
			u8CurrentTransmissionFrame++;
		}
	} else if (MB_PORT_TRANSMIT_BUFFER_FULL()) {
		//Transmission finished
		UCSR0A |= (1 << TXC0);
		MB_START_RECEIVER;
		modbus_state = MB_TURNAROUND; //Wait 3,5 Character
		//Clear Buffer
		u8CurrentTransmissionFrame = 0;
		u8TransmitSizeIndex = 0;
		MB_T35Start();
	}
}

void MB_AddExceptionFrameToTransmitBuffer(MB_EXCEPTION mbExceptionCode, MB_STATE mbState) {
	uint16_t u16CRC = 0;
	ADD_TRANSMIT_BYTE_TO_FRAME(MB_SLAVE_ADDRESS);
	ADD_TRANSMIT_BYTE_TO_FRAME(mbState);
	ADD_TRANSMIT_BYTE_TO_FRAME(ILLEGAL_DATA_VALUE);
	u16CRC = usMBCRC16(u8pTransmitFrame, 3);
	ADD_TRANSMIT_BYTE_TO_FRAME(u16CRC & 0x00FF);
	ADD_TRANSMIT_BYTE_TO_FRAME((u16CRC & 0xFF00) >> 8);
}

int8_t MB_ReadHoldingRegister(MB_STATE currentMB_State) {
	//Activate Function in Main to process Data and receive Transmission Frame
	uint16_t u16QuantityRegister = 0;
	uint16_t u16StartingAddress = 0;
	uint8_t u8ByteCount = 0;
	uint16_t u16CRC = 0;
	u16QuantityRegister = ((uint16_t) u8pReceiveFrame[4] << 8)
			| u8pReceiveFrame[5];
	u16StartingAddress = ((uint16_t) u8pReceiveFrame[2] << 8)
			| u8pReceiveFrame[3];
	u8ByteCount = u16QuantityRegister * 2;

	//ExceptionCode 03
	if (u16QuantityRegister > 0 && u16QuantityRegister <= 0x007D) {
		MB_AddExceptionFrameToTransmitBuffer(ILLEGAL_DATA_VALUE, currentMB_State);
		return -1;
	}

	uint16_t u16StartingAddressLeftData = 0xFFFF - u16StartingAddress;
	if (u16StartingAddressLeftData > u16QuantityRegister) {
		MB_AddExceptionFrameToTransmitBuffer(ILLEGAL_DATA_ADDRESS, currentMB_State);
		return -1;
	}

	//ADD Data to Transmit Frame Buffer
	ADD_TRANSMIT_BYTE_TO_FRAME(MB_SLAVE_ADDRESS);
	ADD_TRANSMIT_BYTE_TO_FRAME(currentMB_State);
	ADD_TRANSMIT_BYTE_TO_FRAME(u8ByteCount);

	//Frame can be maximal 125
	uint8_t u8pRegValue[125];
	MB_PORT_ResponseHoldingRegisters(u8pRegValue,
			u16StartingAddress, u16QuantityRegister);
	uint8_t i = 0;
	while (i < u8ByteCount) {
		ADD_TRANSMIT_BYTE_TO_FRAME(u8pRegValue[i]);
		i++;
	}
	u16CRC = usMBCRC16(u8pTransmitFrame, 3 + u8ByteCount);
	ADD_TRANSMIT_BYTE_TO_FRAME(u16CRC & 0x00FF);
	ADD_TRANSMIT_BYTE_TO_FRAME((u16CRC & 0xFF00) >> 8);

	return 0;
}

int8_t MB_ReadCoil(MB_STATE currentMB_State) {
	uint8_t u8ByteCount = 0;
	uint16_t u16QuantityOfCoils = 0;
	uint16_t u16StartingAddress = 0;
	uint16_t u16CRC = 0;
	u16StartingAddress = u8pReceiveFrame[2] << 8;
	u16StartingAddress &= u8pReceiveFrame[3];
	u16QuantityOfCoils = u8pReceiveFrame[4] << 8;
	u16QuantityOfCoils &= u8pReceiveFrame[5];
	u8ByteCount = u16QuantityOfCoils / 8;

	if ((u16QuantityOfCoils % 8) > 1) {
		u8ByteCount++;
	}
	//Check Exceptions
	//Exception Code 03
	if (u16QuantityOfCoils > 0 && u16QuantityOfCoils <= 2000) {
		// Todo: Exception
	}
	//Exception Code 02
	uint16_t u16BytesLeft = u16StartingAddress - 0xFFFF;
	uint16_t u16BitsLeft = u16BytesLeft * 8;
	if (u16BitsLeft < u16QuantityOfCoils) {
		//Todo: Exception Code 02
	}
	//End Exception

	uint8_t u8pRegValue[125];
	MB_PORT_SendReadCoils(u8pRegValue, u16StartingAddress, u16QuantityOfCoils);

	ADD_TRANSMIT_BYTE_TO_FRAME(MB_SLAVE_ADDRESS);
	ADD_TRANSMIT_BYTE_TO_FRAME(currentMB_State);
	ADD_TRANSMIT_BYTE_TO_FRAME(u8ByteCount);
	uint8_t i = 0;
	while (i < u8ByteCount) {
		ADD_TRANSMIT_BYTE_TO_FRAME(u8pRegValue[i]);
		i++;
	}

	u16CRC = usMBCRC16(u8pTransmitFrame, 3 + u8ByteCount);
	ADD_TRANSMIT_BYTE_TO_FRAME(u16CRC & 0x00FF);
	ADD_TRANSMIT_BYTE_TO_FRAME((u16CRC & 0xFF00) >> 8);
}

void MB_Receive() {
	if (modbus_timer_3_5_is_expired) {
		MB_STATE currentMB_State = u8pReceiveFrame[1];
		uint8_t currentSlaveAddress = u8pReceiveFrame[0];
		uint16_t u16ReceiveFrameSelfCalculatedCRC = 0;
		uint16_t u16ReceiveFrameCRC = 0;

		modbus_timer_3_5_is_expired = 0;
		u8TransmitSizeIndex = 0;

		//Convert the last 2 CRC Bytes into a 16 Bit value
		u16ReceiveFrameCRC = (u8pReceiveFrame[u8ReceiveSizeIndex] << 8) | u8pReceiveFrame[u8ReceiveSizeIndex-1];

		//-2 because the last 2 Bytes are the CRC from the request
		u16ReceiveFrameSelfCalculatedCRC = usMBCRC16(u8pReceiveFrame, u8ReceiveSizeIndex-2);

		//Exception because the calculated and the received CRC value is not the same
		if (u16ReceiveFrameCRC != u16ReceiveFrameSelfCalculatedCRC) {
			MB_START_RECEIVER;
			modbus_state = MB_IDLE;
			return;
		}

		//If Frame is not addressed to me
		if (currentSlaveAddress != MB_SLAVE_ADDRESS) {
			MB_START_RECEIVER;
			modbus_state = MB_IDLE;
			return;
		}

		switch (currentMB_State) {
		case READ_HOLDING_REGISTER:
			MB_ReadHoldingRegister(currentMB_State);
			break;
		case READ_COILS: {
			MB_ReadCoil(currentMB_State);
			break;
		}
		case WRITE_SINGLE_COIL: {
			uint16_t u16OutputAddress = 0;
			uint16_t u16OutputValue = 0;
			ADD_TRANSMIT_BYTE_TO_FRAME(currentMB_State);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[2]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[3]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[4]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[5]);
			// Todo: Exception
			//Exception
			//ExceptionCode 03
			if (u16OutputValue == 0x0000 || u16OutputValue == 0xFF00) {
				// Todo: Exception
			}
			//ExceptionCode 02
			if (u16OutputAddress >= 0x0000) {
			}
			// Todo: MB_PORT_SendWrite_Single_Coil();
			break;
		}
		case WRITE_SINGLE_REGISTER:
			ADD_TRANSMIT_BYTE_TO_FRAME(currentMB_State);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[2]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[3]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[4]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[5]);
			// Todo: Exception
			// Todo: MB_PORT_SendWrite_Single_Register();
			break;
		case WRITE_MULTIPLE_COIL:
			ADD_TRANSMIT_BYTE_TO_FRAME(currentMB_State);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[2]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[3]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[4]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[5]);
			// Todo: Exception
			// Todo: MB_PORT_SendWrite_Multiple_Coils();
			break;
		case WRITE_MULTIPLE_REGISTER:
			ADD_TRANSMIT_BYTE_TO_FRAME(currentMB_State);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[2]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[3]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[4]);
			ADD_TRANSMIT_BYTE_TO_FRAME(u8pReceiveFrame[5]);
			// Todo: Exception
			// Todo: MB_PORT_SendWrite_Multiple_Register();
			break;
		case READ_WRITE_REGISTER:
			ADD_TRANSMIT_BYTE_TO_FRAME(currentMB_State);
			break;
			//Todo: Exception Function Code not accepted
		}
		//Start Transmitter Bus and send Data back
		MB_START_TRANSMITTER;
		modbus_state = MB_Transmit;

		u8ReceiveSizeIndex = 0;
	}
}

void MB_SlavePoll() {
	switch(modbus_state) {
		//MB_MASTER_IDLE
		case MB_IDLE:
			break;
		case MB_TURNAROUND:
			MB_Turnaround();
			break;
		case MB_TX:
			MB_Transmit();
			break;
		case MB_RX:
			MB_Receive();
			break;
	}
}

uint8_t MB_MasterPoll() {
	return 0;
}

void MB_PORT_Timer_35_Expired() {
	modbus_timer_3_5_is_expired = 1;
}

void MB_PORT_Receive_Byte(uint8_t _u8RecByte) {
	MB_PORT_Reset_Timer();
	if(modbus_state == MB_IDLE) {
		MB_T35Start();
		modbus_state = MB_Receive;
	}
	
	if(modbus_state == MB_Receive) {
		u8pReceiveFrame[u8ReceiveSizeIndex++] = _u8RecByte;
	}
}
