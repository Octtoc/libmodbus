/*
 * modbus_rtu.c
 *
 * Created: 13.09.2019 20:37:43
 *  Author: Michael
 */ 

#include "modbus-rtu.h"

void MB_RTUInit() {
	modbus_state = MB_MASTER_IDLE;
	modbus_response_time = 0;
	modbus_timer_1_5 = 0;
	modbus_timer_3_5 = 0;
	u8TransmitSizeIndex = 0;
	u8ReceiveSizeIndex = 0;
	
	RS485ENABLE_DDR |= (1<<RS485REDE_PIN);
	RS485ENABLE_PORT &= ~(1<<RS485REDE_PIN);
}

uint8_t MB_RequestHoldingRegisters(uint8_t _u8DeviceAdress,
					 uint16_t _u16StartingAddress, uint16_t _u16QuantityRegister) {
	if(modbus_state != MB_MASTER_IDLE) {
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
	
	MB_StartTransmitter();
	modbus_state = MB_MASTER_TX;
	modbus_response_time = 0;
	
	return 1;
}

void MB_StartReceiver() {
	RS485ENABLE_PORT &= ~(1<<RS485REDE_PIN);
}

void MB_StartTransmitter() {
	RS485ENABLE_PORT |= (1<<RS485REDE_PIN);
}

void MB_AddTransmitByte(uint8_t _u8Data) {
	u8pTransmitFrame[u8TransmitSizeIndex++] = _u8Data;
}

void MB_AddReceiveByte(uint8_t _u8Data) {
	u8pReceiveFrame[u8ReceiveSizeIndex++] = _u8Data;
}

/*uint8_t MB_GetTransmitByte() {
	static uint8_t u8TransmitFrameIndex = 0;
	return u8pTransmitFrame[u8TransmitFrameIndex++];
}*/

/*uint8_t MB_GetReceiveByte() {
	static uint8_t u8ReceiveFrameIndex = 0;
	return u8pReceiveFrame[u8ReceiveFrameIndex++];
}*/

uint8_t MB_SlavePoll() {
	switch(modbus_state) {
		//MB_MASTER_IDLE
		case MB_MASTER_IDLE:
			break;
		//END MB_MASTER_IDLE
		//MB_MASTER_TURNAROUND
		case MB_MASTER_TURNAROUND:
			if (MB_TimerT35Expired()) {
				MB_T35Reset();
				modbus_state = MB_MASTER_IDLE;
			}
			break;
		//END MB_MASTER_TURNAROUND
		
		//MB_MASTER_TX
		case MB_MASTER_TX:
		{
			//If i can send data
			static uint8_t u8CurrentTransmissionFrame = 0;
			
			if (u8CurrentTransmissionFrame < u8TransmitSizeIndex) {
				if (MB_PORT_Transmit_Byte(u8pTransmitFrame[u8CurrentTransmissionFrame])) {
					u8CurrentTransmissionFrame++;
				}
			}  else if (MB_PORT_TRANSMIT_BUFFER_FULL()) {
				//Transmission finished
				UCSR0A |= (1<<TXC0);
				MB_StartReceiver();
				modbus_state = MB_MASTER_TURNAROUND;	//Wait 3,5 Character
				//Clear Buffer
				u8CurrentTransmissionFrame = 0;
				u8TransmitSizeIndex = 0;
				MB_T35Start();
			}
			break;
		}
		//END MB_MASTER_TX
		//MB_MASTER_RX
		case MB_MASTER_RX:
			if (MB_TimerT35Expired()) {
				MB_T35Reset();
				u8TransmitSizeIndex = 0;
				
				//If Frame address to me
				if (u8pReceiveFrame[0] == MB_SLAVE_ADDRESS) {
					
					switch(u8pReceiveFrame[1]) {
						case READ_HOLDING_REGISTER:
						{
							//Activate Function in Main to process Data and receive Transmission Frame
							uint16_t u16QuantityRegister = 0;
							uint16_t u16StartingAddress = 0;
							uint8_t u8ByteCount = 0;
							uint16_t u16CRC = 0;
							
							u16QuantityRegister = ( (uint16_t)u8pReceiveFrame[4] << 8 ) | u8pReceiveFrame[5];
							u16StartingAddress = ( (uint16_t)u8pReceiveFrame[2] << 8 ) | u8pReceiveFrame[3];
							
							u8ByteCount = u16QuantityRegister * 2;
							
							//ExceptionCode 03
							if (!(u16QuantityRegister > 0 && u16QuantityRegister <= 0x007D)) {
								
							}
							
							u8pTransmitFrame[u8TransmitSizeIndex++] = MB_SLAVE_ADDRESS;
							u8pTransmitFrame[u8TransmitSizeIndex++] = READ_HOLDING_REGISTER;
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8ByteCount;
							
							uint8_t u8pRegValue[125];
							MB_PORT_ResponseHoldingRegisters(u8pRegValue, u16StartingAddress, u16QuantityRegister);
							
							uint8_t i=0;
							while (i < u8ByteCount) {
								u8pTransmitFrame[u8TransmitSizeIndex++] = u8pRegValue[i];
								i++;
							}
							
							u16CRC = usMBCRC16(u8pTransmitFrame, 3+u8ByteCount);
							u8pTransmitFrame[u8TransmitSizeIndex++] = u16CRC & 0x00FF;
							u8pTransmitFrame[u8TransmitSizeIndex++] = (u16CRC & 0xFF00) >> 8;
						}
						break;
						case READ_COILS:
						{
							uint8_t u8ByteCount = 0;
							uint16_t u16QuantityOfCoils = 0;
							uint16_t u16StartingAddress = 0;
							
							u16StartingAddress = u8pReceiveFrame[2] << 8;
							u16StartingAddress &= u8pReceiveFrame[3];
							
							u16QuantityOfCoils = u8pReceiveFrame[4] << 8;
							u16QuantityOfCoils = u16QuantityOfCoils & u8pReceiveFrame[5];
							
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
							
							//Function Code
							u8pTransmitFrame[u8TransmitSizeIndex++] = READ_COILS;
							u8pTransmitFrame[u8TransmitSizeIndex++] = READ_COILS;
							//Todo: MB_PORT_SendReadCoils();
							break;
						}
						case WRITE_SINGLE_COIL:
						{
							uint16_t u16OutputAddress = 0;
							uint16_t u16OutputValue = 0;
							u8pTransmitFrame[u8TransmitSizeIndex++] = WRITE_SINGLE_COIL;
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[2];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[3];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[4];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[5];
							
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
							u8pTransmitFrame[u8TransmitSizeIndex++] = WRITE_SINGLE_COIL;
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[2];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[3];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[4];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[5];
							
							// Todo: Exception
							// Todo: MB_PORT_SendWrite_Single_Register();
						break;
						case WRITE_MULTIPLE_COIL:
							u8pTransmitFrame[u8TransmitSizeIndex++] = WRITE_MULTIPLE_COIL;
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[2];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[3];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[4];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[5];
							
							// Todo: Exception
							// Todo: MB_PORT_SendWrite_Multiple_Coils();
						break;
						case WRITE_MULTIPLE_REGISTER:
							u8pTransmitFrame[u8TransmitSizeIndex++] = WRITE_MULTIPLE_REGISTER;
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[2];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[3];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[4];
							u8pTransmitFrame[u8TransmitSizeIndex++] = u8pReceiveFrame[5];
							
							// Todo: Exception
							// Todo: MB_PORT_SendWrite_Multiple_Register();
						break;
						case READ_WRITE_REGISTER:
							u8pTransmitFrame[u8TransmitSizeIndex++] = READ_WRITE_REGISTER;
						break;
							//Todo: Exception Function Code not accepted
					}
					
					//Todo: Send Transmission Frame to FIFO Buffer and set to TX Mode
					
					//Start RS485 Bus
					MB_StartTransmitter();
					modbus_state = MB_MASTER_TX;		//Now send data back
				} else {
					//Frame is not for me
					MB_StartReceiver();
					modbus_state = MB_MASTER_IDLE;
				}
				u8ReceiveSizeIndex = 0;
			}
			break;
		//END MB_MASTER_RX
	}
	
	return 0;
}

uint8_t MB_MasterPoll() {
	//You can see the state diagram in the modbus application notes
	/*switch(modbus_state) {
		
		case MB_MASTER_TURNAROUND:
		if (MB_TimerT35Expired()) {
			MB_T35Reset();
			modbus_state = MB_MASTER_IDLE;
		}
		break;
		
		case MB_MASTER_IDLE:
		break;
		
		//Transmit Data from FIFO Buffer to RS485
		case MB_MASTER_TX:
		if (UCSRA & (1<<UDRE)) {
			uint8_t u8UartOut;
			if (fiforing_buffer_out(&u8UartOut, &frMBFifoTransmitBuffer) == RINGBUFFER_SUCCESS) {
				MB_PORT_Transmit_Byte(u8UartOut);
			} else if (UCSRA & (1<<TXC)) {
				//Transmission finished
				UCSRA |= (1<<TXC);	//Clear TXC Flag UCSRA
				MB_StartReceiver();
				modbus_state = MB_MASTER_WAIT_RESPONSE;
			}
		}
		break;
		
		case MB_MASTER_WAIT_RESPONSE:
		if (modbus_response_time > MB_MAX_RESPONSE_TIME_MS) {
			modbus_response_time = 0;
			modbus_state = MB_MASTER_IDLE;
			//Error
		}
		break;
		
		//Receive Data from RS485 to FIFO Buffer
		case MB_MASTER_RX:
		if (MB_TimerT35Expired()) {
			MB_T35Reset();
			MB_ResponseHoldingRegisters(u8pReceiveFrame);
			MB_PORT_ReadHoldingRegisters(u8pReceiveFrame);
			
			MB_StartTransmitter();
			modbus_state = MB_MASTER_TURNAROUND;	//Wait 3,5 Character
			MB_T35Start();
		}
		break;
	}
	
	return 0;*/
}

void MB_T35Start() {
	MB_T35Reset();
	TCNT1 = 0;
}

void MB_T35Reset() {
	modbus_timer_3_5 = 0;
}

uint8_t MB_TimerT35Expired() {
	return modbus_timer_3_5;
}

void MB_PORT_Timer_35() {
	//When Timer expired Set Value to 1
	modbus_timer_3_5 = 1;
}

void MB_PORT_Receive_Byte(uint8_t _u8RecByte) {
	//Reset Timer Counter
	MB_PORT_Reset_Timer();
	if(modbus_state == MB_MASTER_IDLE) {
		MB_T35Start();
		modbus_state = MB_MASTER_RX;
	}
	
	if(modbus_state == MB_MASTER_RX) {
		u8pReceiveFrame[u8ReceiveSizeIndex++] = _u8RecByte;
	}
}