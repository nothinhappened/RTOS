/*
 * CFile1.c
 *
 * Created: 02/04/2015 9:11:33 PM
 *  Author: lbissonnette
 */ 

#include "../roomba/roomba.h"
#include "../oskernel.h"
#include "../ir/ir.h"
#include "../radio/packet.h"
#include "../radio/radio.h"
#include <util/delay.h>

enum speed { OFF = 0, VERY_SLOW = 100, SLOW = 200, NORMAL = 250, FAST = 300, VERY_FAST = 500 };

pf_gamestate_t network_queue[10];

pf_gamestate_t* queue_tail = network_queue;
pf_gamestate_t* queue_head = NULL;
pf_gamestate_t* queue_end = &(network_queue[10]);

void push_network_queue(pf_gamestate_t packet);
pf_gamestate_t pop_network_queue();
int isEmpty_network_queue();
void sys_play_prelude();

GAME_STATE game_state = GAME_STARTING;

int myState = ALIVE;

volatile int speed = 0;
volatile int angle = 0;
volatile char flag_radio = 0;
volatile uint8_t shot_by = 0;
volatile char flag_ir = 0;


void radio_rxhandler(uint8_t pipenumber) {
	radiopacket_t buffer;
	RADIO_RX_STATUS status;
	do {
		status = Radio_Receive(&buffer);
		if (status != RADIO_RX_SUCCESS) push_network_queue(buffer.payload.gamestate);
	} while (status != RADIO_RX_FIFO_EMPTY);
	flag_radio=1;
}

void ir_rxhandler(uint8_t byte) {
	flag_ir = 1;
	shot_by = byte;
	Roomba_Drive(400,-1);
}

void sys_game_over() {
	// check if alive
	if (myState == ALIVE) {
		Task_Create_System(sys_play_prelude,0);
	}
}

void rr_startRotate() {
	speed = NORMAL;
	angle = -1;
	Roomba_Drive(speed,angle);
	Task_Terminate();
}

void rr_stopRotate() {
	speed = OFF;
	angle = -1;
	Roomba_Drive(speed, angle);
	Task_Terminate();
}

void pp_shoot() {
	while(1) {
		if (myState == ALIVE) {
			Roomba_Drive(0,0);
			IR_transmit(ROBBER_CODE);
			Roomba_Drive(speed, angle);
		}
		Task_Next();
	}
}

void revive() {
	myState = ALIVE;
	Task_Create_RR(rr_startRotate,0);
}

void die() {
	myState = DEAD;
	Task_Create_RR(rr_stopRotate,0);
}

void pp_handlePacket() {
	while(1) {
		if (flag_radio) {
			while (!isEmpty_network_queue()) {
				pf_gamestate_t state = pop_network_queue();
				if (game_state != (GAME_STATE)  state.game_state) {
					GAME_STATE old_game_state = game_state;
					game_state = (GAME_STATE) state.game_state;
					switch (game_state) {
						case GAME_RUNNING:
							revive();
							if (old_game_state == GAME_STARTING) Task_Create_Periodic(pp_shoot,0,100,25,20);
							break;
						case GAME_OVER:
							Task_Create_System(sys_game_over, 0);
					}
					if (myState != state.roomba_states[ROOMBA_NUM] & ~FORCED) {
						// Geoff and I disagree with the state we're in
						if (state.roomba_states[ROOMBA_NUM] & FORCED) {
							// Geoff is forcing us into a state
							if (state.roomba_states[ROOMBA_NUM] & DEAD) {
								// Geoff wants us dead
								myState = DEAD;
							} else {
								myState = ALIVE;
							}
						} else {
							// Geoff is not forcing us and we disagree
							// We need to correct Geoff
							radiopacket_t packet;
							packet.payload.roombastate.roomba_id = ROOMBA_NUM;
							packet.payload.roombastate.roomba_state = myState;
							packet.type = ROOMBASTATE_PACKET;
							Radio_Transmit(&packet,RADIO_RETURN_ON_TX);
						}
					}
				}
			}
		}
		Task_Next();
	}
}

void pp_handle_ir() {
	_delay_ms(1);
	if(flag_ir) {
			Task_Create_System(rr_startRotate,0);
		if (shot_by = ROBBER_CODE && myState == DEAD) {
			revive();
		} else {
			die();
		}
	}
}

void push_network_queue(pf_gamestate_t packet) {
	*queue_tail = packet;
	
	if (!queue_head) queue_head = queue_tail;
	
	if (queue_tail == queue_end) queue_tail = network_queue;
	else queue_tail++;
}

pf_gamestate_t pop_network_queue() {
	pf_gamestate_t state;
	
	if (queue_head) {
		state = *queue_head;
	
		if (queue_head == queue_end) queue_head = network_queue;
		else queue_head++;
	
		if (queue_head == queue_tail) queue_head = NULL;
	}
	return state;
}

int isEmpty_network_queue() {
	return !queue_head;
}

void setup_songs() {
	char prelude_pt_1_notes[32] = {
		46, 16, 
		36, 16, 
		38, 16, 
		41, 16,
		46, 16,
		48, 16,
		50, 16,
		53, 16,
		58, 16,
		60, 16,
		62, 16,
		65, 16,
		70, 16,
		72, 16,
		74, 16,
		77, 16
		};
	char prelude_pt_2_notes[32] = {
		82, 16,
		77, 16,
		74, 16,
		72, 16,
		70, 16,
		65, 16,
		62, 16,
		60, 16,
		58, 16,
		53, 16,
		50, 16,
		48, 16,
		46, 16,
		41, 16,
		38, 16,
		36, 16
	};
	char prelude_pt_3_notes[32] = {
		43, 16,
		45, 16,
		46, 16,
		38, 16,
		43, 16,
		45, 16,
		46, 16,
		50, 16,
		55, 16,
		57, 16,
		58, 16,
		62, 16,
		67, 16,
		69, 16,
		70, 16,
		74, 16
	};
	char prelude_pt_4_notes[32] = {
		79, 16,
		74, 16,
		70, 16,
		69, 16,
		67, 16,
		62, 16,
		58, 16,
		57, 16,
		55, 16,
		50, 16,
		46, 16,
		45, 16,
		43, 16,
		38, 16,
		46, 16,
		45, 16
	};
	note_t* noteptr_prelude_pt_1 = Roomba_Write_Song(prelude_pt_1_notes, 32);
	note_t* noteptr_prelude_pt_2 = Roomba_Write_Song(prelude_pt_2_notes, 32);
	note_t* noteptr_prelude_pt_3 = Roomba_Write_Song(prelude_pt_3_notes, 32);
	note_t* noteptr_prelude_pt_4 = Roomba_Write_Song(prelude_pt_4_notes, 32);

	song_t prelude_pt_1 = {0, 16, noteptr_prelude_pt_1};
	song_t prelude_pt_2 = {1, 16, noteptr_prelude_pt_2};
	song_t prelude_pt_3 = {2, 16, noteptr_prelude_pt_3};
	song_t prelude_pt_4 = {3, 16, noteptr_prelude_pt_4};
		
	Roomba_Song(prelude_pt_1);
	Roomba_Song(prelude_pt_2);
	Roomba_Song(prelude_pt_3);
	Roomba_Song(prelude_pt_4);
}

/* Task which plays the first four bars of the prelude from Final Fantasy 1
 *
 * play time: 16 seconds (wcet of 16000 ms + 200ms wiggle room / 5 ms/tick  = 3240 ticks)
 *
 */
void play_prelude() {
	Roomba_Play(0);
	_delay_ms(16*16/64 * 1000);
	Roomba_Play(1);
	_delay_ms(16*16/64 * 1000);
	Roomba_Play(2);
	_delay_ms(16*16/64 * 1000);
	Roomba_Play(3);
}

/*
void sys_test_ir() {
	while(1){
		IR_transmit(ROBBER_CODE);
		//IR_transmit(0xFF);
		Task_Next();
	}
}*/

int r_main(void) {
	Roomba_Init();
	setup_songs();
	play_prelude();
	//Task_Create_System(rr_stopRotate,0);
	//Task_Create_RR(sys_test_ir,0);
	//Task_Create_Periodic(pp_handlePacket,0,200,2,0);
	Task_Create_Periodic(pp_handle_ir,0,50,10,10);
	//Task_Create_Periodic(pp_shoot,0,50,30,20);
	Enable_Interrupt();
	return 0;
}