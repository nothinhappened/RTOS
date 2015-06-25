/*
 * roomba.c
 *
 *  Created on: 4-Feb-2009
 *      Author: nrqm
 */

#include "roomba_sci.h"
#include "../uart/uart.h"
#include <util/delay.h>
#include "avr/interrupt.h"
#include "../radio/radio.h"
#include "../cops_and_robbers.h"
#include "../ir/ir.h"
#include "roomba.h"

//#define     clock8MHz()    CLKPR = _BV(CLKPCE); CLKPR = 0x00;
#define		TEST_UNIT 10
void Roomba_Init()
{
	cli();
			
	DDRD = 0xFF;
	PORTD = 0x00;
		
	_delay_ms(500);
	PORTD |= 1<<PD1;
	DDRC = 0xFF;
	PORTC |= _BV(PC2) |  _BV(PC3) |_BV(PC4) |_BV(PC5) ;
		
	DDRL |= (1 << PL2);
	PORTL &= ~(1<<PL2);
	_delay_ms(500);
	PORTL |= (1<<PL2);
	_delay_ms(500);
	
	// At 8 MHz, the AT90 generates a 57600 bps signal with a framing error rate of over 2%, which means that more than
	// 1 out of every 50 bits is wrong.  The fastest bitrate with a low error rate that the Roomba supports is
	// 38400 bps (0.2% error rate, or 1 bit out of every 500).

	// Try 57.6 kbps to start (this is the Roomba's default baud rate after the battery is installed).
	Roomba_UART_Init(UART_115200);

	// Try to start the SCI
	Roomba_Send_Byte(START);
	_delay_ms(20);

	// change the baud rate to 38400 bps.  Have to wait for 100 ms after changing the baud rate.
	Roomba_Send_Byte(BAUD);
	Roomba_Send_Byte(ROOMBA_19200BPS);
	_delay_ms(100);		// this delay will not work on old versions of WinAVR (new versions will see reduced but
						// still acceptable resolution; see _delay_ms definition)

	// change the AT90's UART clock
	Roomba_UART_Init(UART_19200);

	// start the SCI again in case the first start didn't go through.
	Roomba_Send_Byte(START);
	_delay_ms(20);

	// finally put the Roomba into safe mode.
	Roomba_Send_Byte(SAFE);
	_delay_ms(20);
	
	IR_init();
		
	_delay_ms(2000);
	Radio_Init();

	Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[ROOMBA_NUM], ENABLE);
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
	
	//Radio_Set_Tx_Addr(BASE_ADDRESS);

	sei();
}

void Roomba_Finish() {
	Roomba_Send_Byte(STOP);
}

void Roomba_UpdateSensorPacket(ROOMBA_SENSOR_GROUP group, roomba_sensor_data_t* sensor_packet)
{
	Roomba_Send_Byte(SENSORS);
	Roomba_Send_Byte(group);
	switch(group)
	{
	case EXTERNAL:
		// environment sensors
		while (uart_bytes_received() != 10);
		sensor_packet->bumps_wheeldrops = uart_get_byte(0);
		sensor_packet->wall = uart_get_byte(1);
		sensor_packet->cliff_left = uart_get_byte(2);
		sensor_packet->cliff_front_left = uart_get_byte(3);
		sensor_packet->cliff_front_right = uart_get_byte(4);
		sensor_packet->cliff_right = uart_get_byte(5);
		sensor_packet->virtual_wall = uart_get_byte(6);
		sensor_packet->motor_overcurrents = uart_get_byte(7);
		sensor_packet->dirt_left = uart_get_byte(8);
		sensor_packet->dirt_right = uart_get_byte(9);
		break;
	case CHASSIS:
		// chassis sensors
		while (uart_bytes_received() != 6);
		sensor_packet->remote_opcode = uart_get_byte(0);
		sensor_packet->buttons = uart_get_byte(1);
		sensor_packet->distance.bytes.high_byte = uart_get_byte(2);
		sensor_packet->distance.bytes.low_byte = uart_get_byte(3);
		sensor_packet->angle.bytes.high_byte = uart_get_byte(4);
		sensor_packet->angle.bytes.low_byte = uart_get_byte(5);
		break;
	case INTERNAL:
		// internal sensors
		while (uart_bytes_received() != 10);
		sensor_packet->charging_state = uart_get_byte(0);
		sensor_packet->voltage.bytes.high_byte = uart_get_byte(1);
		sensor_packet->voltage.bytes.low_byte = uart_get_byte(2);
		sensor_packet->current.bytes.high_byte = uart_get_byte(3);
		sensor_packet->current.bytes.low_byte = uart_get_byte(4);
		sensor_packet->temperature = uart_get_byte(5);
		sensor_packet->charge.bytes.high_byte = uart_get_byte(6);
		sensor_packet->charge.bytes.low_byte = uart_get_byte(7);
		sensor_packet->capacity.bytes.high_byte = uart_get_byte(8);
		sensor_packet->capacity.bytes.low_byte = uart_get_byte(9);
		break;
	}
	uart_reset_receive();
}

void Roomba_Drive( int16_t velocity, int16_t radius )
{
	Roomba_Send_Byte(DRIVE);
	Roomba_Send_Byte(HIGH_BYTE(velocity));
	Roomba_Send_Byte(LOW_BYTE(velocity));
	Roomba_Send_Byte(HIGH_BYTE(radius));
	Roomba_Send_Byte(LOW_BYTE(radius));
}


void Roomba_Song(song_t song) {
	int i;
	Roomba_Send_Byte(SONG);
	Roomba_Send_Byte(song.song_number);
	Roomba_Send_Byte(song.song_length);
	
	for( i = 0; i < song.song_length; i++) {
		Roomba_Send_Byte(song.notes[i].note);
		Roomba_Send_Byte(song.notes[i].duration);
	}
}

note_t* Roomba_Write_Song(char* notes_and_durations, int length) {
	int i;
	int counter = 0;
	int note_count = length/2;
	if(length % 2 == 0) {
		note_t notes[note_count];
		for( i = 0; i < note_count; i++) {
			notes[i].note = notes_and_durations[counter++];
			notes[i].duration = notes_and_durations[counter++];
			
		}
		return notes;
	}
	return NULL;
}

void Roomba_Play(int16_t song_number) {
	Roomba_Send_Byte(PLAY);
	Roomba_Send_Byte(song_number);
}

