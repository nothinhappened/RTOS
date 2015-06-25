/*
 * uart.h
 *
 * Created: 25/01/2015 12:53:55 PM
 *  Author: Daniel
 */ 


#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#define UART_BUFFER_SIZE    32

void Roomba_Send_Byte(uint8_t data_out);

typedef enum _uart_bps
{
	UART_19200,
	UART_38400,
	UART_57600,
	UART_115200,
	UART_DEFAULT,
} UART_BPS;

void Roomba_UART_Init(UART_BPS baud);
uint8_t uart_bytes_received(void);
void uart_reset_receive(void);
uint8_t uart_get_byte(int index);

#endif /* UART_H_ */