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
		MB_PORT_Flush_Buffer();
		MB_START_RECEIVER;
		modbus_state = MB_TURNAROUND; //Wait 3,5 Character
		TransmitFrame.frameIndex = 0;
		TransmitFrame.frameMaxCounter = 0;
		MB_PORT_Reset_Timer();
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
		u16ReceiveFrameCRC = ((uint16_t)ReceiveFrame.frameField[ReceiveFrame.frameMaxCounter-1] << 8)
						| ReceiveFrame.frameField[ReceiveFrame.frameMaxCounter-2];

		//-2 because the last 2 Bytes are the CRC from the request
		u16ReceiveFrameSelfCalculatedCRC = usMBCRC16(ReceiveFrame.frameField, ReceiveFrame.frameMaxCounter-2);

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
				uint8_t checkExceptionCode;
				checkExceptionCode = MB_FillHoldingRegister(&holding_register, &ReceiveFrame);
				if (checkExceptionCode) {
					mb_exception_t exception;
					exception.exceptionCode = checkExceptionCode;
					exception.functionCode = READ_HOLDING_REGISTER;
					MB_AddExceptionToFrame(&exception, &TransmitFrame);
				} else {
					MB_AddHoldingRegisterToFrame(&holding_register, &TransmitFrame);
				}
			} break;
			case READ_COILS: {
				mb_coil_t coil;
				MB_FillReadCoil(&coil, &ReceiveFrame);
				MB_AddReadCoilToFrame(&coil, &TransmitFrame);
			} break;
			case WRITE_SINGLE_COIL: {
				mb_write_coil_t write_coil;
				MB_FillWriteCoil(&write_coil, &ReceiveFrame);
				MB_AddWriteCoilToFrame(&write_coil, &TransmitFrame);
			} break;
			default: {
				mb_exception_t exception;
				exception.exceptionCode = ILLEGAL_FUNCTION;
				exception.functionCode = currentMB_State;
				MB_AddExceptionToFrame(&exception, &TransmitFrame);
			}

		}
		//Start Transmitter Bus and send Data back
		MB_START_TRANSMITTER;
		modbus_state = MB_TX;
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
		ReceiveFrame.frameIndex = 0;
		ReceiveFrame.frameMaxCounter = 0;
	}
	
	if(modbus_state == MB_RX) {
		MB_PORT_Reset_Timer();

		ReceiveFrame.frameField[ReceiveFrame.frameMaxCounter++] = _u8RecByte;
	}
}
