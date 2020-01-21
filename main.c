/*
 * GccApplication9.c
 *
 * Created: 02.01.2020 17:06:51
 * Author : Michael
 */

#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#define BAUD 19200
#define DATABITS 10
#define PRESCALER 256
#include <util/delay.h>
#include "modbus-rtu.h"

#if BAUD < 19200
#define OCNRX_VALUE ((F_CPU*3.5*DATABITS)/(PRESCALER*BAUD)-1)
#else
#define OCNRX_VALUE ((F_CPU*3.5*DATABITS)/(PRESCALER*19200UL)-1)
#endif

void UartInit(void);
void TimerInit(void);

int main(void)
{
    DDRB = (1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB2);
    PORTB = (1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB2);

	DDRD = (1<<PD3);
	PORTD = (1<<PD3);

	DDRC = (1<<PC5);

	UartInit();
	TimerInit();
	MB_RTUInit();

	sei();

    while (1)
    {
		MB_SlavePoll();
    }
}

void UartInit(void) {
	#include <util/setbaud.h>

	UBRR0L = UBRRL_VALUE;
	UBRR0H = UBRRH_VALUE;

	UCSR0B |= (1 << TXEN0) | (1 << RXEN0) | (1<<RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void TimerInit(void) {
	TCCR1B |= (1<<WGM12) | (1<<CS12);
	OCR1A = OCNRX_VALUE;
	TCNT1 = 0;
	TIMSK1 |= (1<<OCIE1A);
}

uint8_t MB_PORT_Transmit_Byte(uint8_t u8TrByte) {
	if (UCSR0A & (1<<UDRE0)) {
		UDR0 = u8TrByte;
		return 1;
	}
	return 0;
}
void MB_PORT_ResponseHoldingRegisters(mb_holding_register_t *holding_register) {
	holding_register->register_value[0] = 0x35;
	holding_register->register_value[1] = 0x34;
	holding_register->register_value[2] = 0x33;
	holding_register->register_value[3] = 0x32;
}
void MB_PORT_SendReadCoils(mb_coil_t *function_coil) {
	function_coil->coil_status[0] = 0x02;
}
uint8_t MB_PORT_TRANSMIT_BUFFER_FULL() {
	return (UCSR0A & (1<<TXC0));
}
void MB_PORT_Reset_Timer(void) {
	TCNT1 = 0;
}
void MB_PORT_Flush_Buffer() {
	UCSR0A |= (1 << TXC0);
}

ISR(TIMER1_COMPA_vect) {
	MB_PORT_Timer_35_Expired();
}

ISR(USART_RX_vect) {
	MB_PORT_Receive_Byte(UDR0);
}
