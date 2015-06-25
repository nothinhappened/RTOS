#include "uart.h"

#define UART_BUFFER_SIZE    32

static volatile uint8_t uart_buffer[UART_BUFFER_SIZE];
static volatile uint8_t uart_buffer_index;

void Roomba_Send_Byte(uint8_t data_out){
	while(! (UCSR1A & (1<<UDRE1)));
	UDR1 = data_out;
}

void Roomba_UART_Init(UART_BPS baud){
	uint8_t sreg = SREG;
	cli();
	
	// Make sure I/O clock to USART1 is enabled
	PRR1 &= ~(1 << PRUSART1);
	
	// Set baud rate to 19.2k at fOSC = 16 MHz
	
	switch(baud) {
		case UART_19200:
			UBRR1 = 51;
			break;
		case UART_38400:
			UBRR1 = 25;
			break;
		case UART_57600:
			UBRR1 = 16;
			break;
		case UART_115200:
			UBRR1 = 8;
			break;
		default:
			UBRR1 = 16;
	}
	
	// Clear USART Transmit complete flag, normal USART transmission speed
	UCSR1A = (1 << TXC1) | (0 << U2X1);
	
	// Enable receiver, transmitter, and rx complete interrupt.
	UCSR1B = (1<<TXEN1);
	// 8-bit data
	UCSR1C = ((1<<UCSZ11)|(1<<UCSZ10));
	// disable 2x speed
	UCSR1A &= ~(1<<U2X1);
	
	
	SREG = sreg;
}

uint8_t uart_bytes_received(void)
{
	return uart_buffer_index;
}

void uart_reset_receive(void)
{
	uart_buffer_index = 0;
}

/**
 * UART receive byte ISR
 */
ISR(USART1_RX_vect)
{
    uart_buffer[uart_buffer_index] = UDR1;
    uart_buffer_index = (uart_buffer_index + 1) % UART_BUFFER_SIZE;
}

uint8_t uart_get_byte(int index)
{
	if (index < UART_BUFFER_SIZE)
	{
		return uart_buffer[index];
	}
	return 0;
}