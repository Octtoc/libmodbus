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
#include "modbus-functions.h"

#define MB_MAX_RESPONSE_TIME_MS		1000
#define MB_SLAVE_ADDRESS			0x01

#define RS485_TX_RX_SWITCH_DDR	DDRD
#define RS485_TX_RX_SWITCH_PORT	PORTD
#define RS485_TX_RX_SWITCH_PIN	PD3

#define ADD_TRANSMIT_BYTE_TO_FRAME(X) (u8pTransmitFrame[u8TransmitSizeIndex++] = (X))
#define ADD_RECEIVE_BYTE_TO_FRAME(X) (u8pReceiveFrame[u8ReceiveSizeIndex++] = (X))
#define MB_START_TRANSMITTER (RS485_TX_RX_SWITCH_PORT |= (1<<RS485_TX_RX_SWITCH_PIN))
#define MB_START_RECEIVER (RS485_TX_RX_SWITCH_PORT &= ~(1<<RS485_TX_RX_SWITCH_PIN))

enum {
	MB_TURNAROUND,
	MB_IDLE,
	MB_TX,
	MB_RX,
	MB_WAIT_RESPONSE
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

enum {
	ILLEGAL_FUNCTION					= 0x01,
	ILLEGAL_DATA_ADDRESS				= 0x02,
	ILLEGAL_DATA_VALUE					= 0x03,
	SERVER_DEVICE_FAILURE				= 0x04,
	ACKNOWLEDGE							= 0x05,
	SERVER_DEVICE_BUSY					= 0x06,
	MEMORY_PARITY_ERROR					= 0x08,
	GATEWAY_PATH_UNAVAILABLE			= 0x0A,
	GATEWAY_TARGET_DEVICE_FAILED_RESP	= 0x0B
};
typedef uint8_t MB_EXCEPTION;

typedef struct {
	uint8_t frameIndex;
	uint8_t frameMaxCounter;
	uint8_t frame[128];
} frame;

volatile MB_STATE modbus_state;
volatile uint16_t modbus_response_time;
volatile uint8_t modbus_timer_1_5_is_expired;
volatile uint8_t modbus_timer_3_5_is_expired;
volatile frame ReceiveFrame[128];
volatile frame TransmitFrame[128];

void MB_RTUInit();
uint8_t MB_RequestHoldingRegisters(uint8_t _u8DeviceAdress, uint16_t _u16StartingAddress, uint16_t _u16QuantityRegister);
void MB_SlavePoll();
uint8_t MB_MasterPoll();
void MB_StartReceiver();
void MB_StartTransmitter();

void MB_Turnaround();
void MB_Receive();
void MB_Transmit();

uint8_t MB_PORT_TRANSMIT_BUFFER_FULL();
void MB_PORT_Timer_35_Expired();
uint8_t MB_PORT_Transmit_Byte(uint8_t u8TrByte);
void MB_PORT_Receive_Byte(uint8_t _u8RecByte);
void MB_PORT_Reset_Timer();

void MB_PORT_ResponseHoldingRegisters(mb_function_holding_register *holding_register);
void MB_PORT_SendReadCoils(mb_function_coil *function_coil);

#endif /* MODBUS_RTU_H_ */
