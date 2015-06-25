/*
 * ir.h
 *
 * Created: 2015-01-28 12:15:37
 *  Author: Daniel
 */ 


#ifndef IR_H_
#define IR_H_

#include "avr/io.h"

//Arbitrary random values which are fairly distinct in binary.
typedef enum _ir_command_bytes {IR_SHOOT = 0x92, IR_WAKE_COP1 = 0x24, IR_WAKE_COP2 = 0x11, IR_WAKE_ROBBER1 = 0x84, IR_WAKE_ROBBER2 = 0xE3} IR_COMMAND_CODE;

void IR_transmit(uint8_t data);
void IR_init();
uint8_t IR_getLast();



#endif /* IR_H_ */