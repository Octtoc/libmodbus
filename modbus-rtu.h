/*
 * modbus_rtu.h
 *
 * Created: 13.09.2019 20:37:38
 *  Author: Michael
 */ 


#ifndef MODBUS_RTU_H_
#define MODBUS_RTU_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include "modbus-crc16.h"

#define MB_MAX_RESPONSE_TIME_MS		1000
#define MB_SLAVE_ADDRESS			0x01

#define RS485ENABLE_DDR		DDRD
#define RS485ENABLE_PORT	PORTD
#define RS485REDE_PIN		PD3

enum {
	MB_MASTER_TURNAROUND,
	MB_MASTER_IDLE,
	MB_MASTER_TX,
	MB_MASTER_RX,
	MB_MASTER_WAIT_RESPONSE
};
typedef uint8_t MB_STATE;

enum {
	READ_DISCRETE_INPUTS		= 0x02,
	READ_COILS					= 0x01,
	WRITE_SINGLE_COIL			= 0x05,
	WRITE_MULTIPLE_COIL			= 0x0F,
	READ_INPUT_REGISTER			= 0x04,
	READ_HOLDING_REGISTER		= 0x03,
	WRITE_SINGLE_REGISTER		= 0x06,
	WRITE_MULTIPLE_REGISTER		= 0x10,
	READ_WRITE_REGISTER			= 0x17,
	MASK_WRITE_REGISTER			= 0x16,
	READ_FIFO_QUEUE				= 0x18,
	READ_FILE_RECORD			= 0x14,
	WRITE_FILE_RECORD			= 0x015,
	READ_EXCEPTION_STATUS		= 0x97,
	DIAGNOSTIC					= 0x08,
	GET_COM_EVENT_COUNTER		= 0x0B,
	GET_COM_EVENT_LOG			= 0x0C,
	REPORT_SERVER_ID			= 0x11,
	READ_DEV_IDENT				= 0x2B,
	ENC_INT_TRANSPORT			= 0x2B,
	CANOPEN_GENERAL_REF			= 0x2B
};
typedef uint8_t MB_FUNCTION;
volatile MB_STATE modbus_state;

uint16_t modbus_response_time;
uint8_t modbus_timer_1_5;
uint8_t modbus_timer_3_5;
volatile uint8_t u8TransmitSizeIndex;
volatile uint8_t u8ReceiveSizeIndex;
volatile uint8_t u8pReceiveFrame[128];
volatile uint8_t u8pTransmitFrame[128];

void MB_RTUInit();
uint16_t MB_CalculateCRC();
uint8_t MB_RequestHoldingRegisters(uint8_t _u8DeviceAdress, uint16_t _u16StartingAddress, uint16_t _u16QuantityRegister);
void MB_ResponseHoldingRegisters(uint8_t *_u8fResponse);
void MB_SendResponse(uint8_t *_u8pData);
uint8_t MB_SlavePoll();
uint8_t MB_MasterPoll();
uint8_t MB_TimerT35Expired();
void MB_T35Reset();
void MB_T35Start();
uint8_t MB_TimerReset();
void MB_StartReceiver();
void MB_StartTransmitter();

uint8_t MB_PORT_TRANSMIT_BUFFER_FULL();
void MB_PORT_Timer_35();
uint8_t MB_PORT_Transmit_Byte(uint8_t u8TrByte);
void MB_PORT_Receive_Byte(uint8_t _u8RecByte);
void MB_PORT_Reset_Timer();
void MB_PORT_ResponseHoldingRegisters(uint8_t *_u8pRegisterValue, uint16_t u16StartingAddress, uint8_t u16QuantityRegister);

#endif /* MODBUS_RTU_H_ */
