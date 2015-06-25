/*
 * IncFile1.h
 *
 * Created: 02/04/2015 9:11:48 PM
 *  Author: lbissonnette
 */ 


#ifndef INCFILE1_H_
#define INCFILE1_H_

int r_main();

void radio_rxhandler(uint8_t pipenumber);
void ir_rxhandler(uint8_t byte);

#endif /* INCFILE1_H_ */