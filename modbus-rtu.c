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
	
	uint8_t u8iCountTransmit;
	for(u8iCountTransmit = 0; u8iCountTransmit < 8; u8iCountTransmit++) {
		//MB_TransmitByteFIFO(u8Frame[u8iCountTransmit]);
	}
	
	MB_START_TRANSMITTER;
	modbus_state = MB_TX;
	modbus_response_time = 0;
	
	return 1;
}

void MB_Turnaround() {
	if (modbus_timer_3_5_is_expired) {
		modbus_timer_3_5_is_expired = 0;
		modbus_state = MB_IDLE;
	}
}

void MB_Transmit() {
	if (TransmitFrame.frameIndex < TransmitFrame.frameMaxCounter) {
		if (MB_PORT_Transmit_Byte(TransmitFrame.frameField[TransmitFrame.frameIndex])) {
			TransmitFrame.frameIndex++;
		}
	} else if (MB_PORT_TRANSMIT_BUFFER_FULL()) {
		//Transmission finished
		UCSR0A |= (1 << TXC0);
		MB_START_RECEIVER;
		modbus_state = MB_TURNAROUND; //Wait 3,5 Character
		TransmitFrame.frameIndex = 0;
		TransmitFrame.frameMaxCounter = 0;
		modbus_timer_3_5_is_expired = 0;
	}
}

void MB_Receive() {
	if (modbus_timer_3_5_is_expired) {
		MB_STATE currentMB_State = ReceiveFrame.frameField[1];
		uint8_t currentSlaveAddress = ReceiveFrame.frameField[0];
		uint16_t u16ReceiveFrameSelfCalculatedCRC = 0;
		uint16_t u16ReceiveFrameCRC = 0;

		modbus_timer_3_5_is_expired = 0;

		//Convert the last 2 CRC Bytes into a 16 Bit value
		u16ReceiveFrameCRC = (ReceiveFrame.frameField[ReceiveFrame.frameMaxCounter] << 8) | ReceiveFrame.frameField[ReceiveFrame.frameMaxCounter-1];

		//-2 because the last 2 Bytes are the CRC from the request
		u16ReceiveFrameSelfCalculatedCRC = usMBCRC16(ReceiveFrame.frameField, ReceiveFrame.frameMaxCounter-2);

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
		case READ_HOLDING_REGISTER: {
			mb_holding_register_t holding_register;
			MB_FillHoldingRegister(&holding_register, &ReceiveFrame);
			MB_AddHoldingRegisterToFrame(&holding_register, &TransmitFrame);

			break;
		}
		case READ_COILS: {
			mb_coil_t coil;
			MB_FillReadCoil(&coil, &ReceiveFrame);
			MB_AddReadCoilToFrame(&coil, &TransmitFrame);
			break;
		}
		case WRITE_SINGLE_COIL: {
			break;
		}
		case WRITE_SINGLE_REGISTER: {
			break;
		}
		case WRITE_MULTIPLE_COIL: {
			break;
		}
		case WRITE_MULTIPLE_REGISTER: {
			break;
		}
		case READ_WRITE_REGISTER: {
			break;
		}

		//Start Transmitter Bus and send Data back
		MB_START_TRANSMITTER;
		modbus_state = MB_TX;

		ReceiveFrame.frameIndex = 0;
		ReceiveFrame.frameMaxCounter = 0;
	}
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
	if(modbus_state == MB_IDLE) {
		modbus_timer_3_5_is_expired = 0;
		modbus_state = MB_RX;
	}
	
	if(modbus_state == MB_RX) {
		modbus_timer_3_5_is_expired = 0;
		MB_PORT_Reset_Timer();
		ReceiveFrame.frameField[ReceiveFrame.frameMaxCounter++] = _u8RecByte;
	}
}
