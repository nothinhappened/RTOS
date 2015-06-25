/*
 * cops_and_robbers.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Daniel
 */


#ifndef COPS_AND_ROBBERS_H_
#define COPS_AND_ROBBERS_H_

#include "avr/io.h"

#define ALIVE 0
#define DEAD 1 << 0
#define FORCED 1 << 1

typedef enum _game_state {
  GAME_STARTING,
  GAME_RUNNING,
  GAME_OVER
} GAME_STATE;

typedef enum _cops_and_robbers {
  COP1 = 0,
  COP2 = 1,
  ROBBER1 = 2,
  ROBBER2 = 3
} COPS_AND_ROBBERS;

typedef enum _ir_teams {
  COP_CODE = (uint8_t)'B',
  ROBBER_CODE = (uint8_t)'A'
} IR_TEAM_CODE;

typedef struct _gs_pkt
{
  uint8_t game_state; // GAME_STATE
  uint8_t roomba_states[4]; // DEAD | FORCED
} pf_gamestate_t;

/// Packet for roomba response if the gamestate is incorrect.
typedef struct _roomba_pkt
{
  uint8_t roomba_id; // COPS_AND_ROBBERS
  uint8_t roomba_state; // DEAD | FORCED
} pf_roombastate_t;

extern uint8_t BASE_ADDRESS[5];
extern uint8_t ROOMBA_ADDRESSES[][5];

extern uint8_t BASE_FREQUENCY;
extern uint8_t ROOMBA_FREQUENCIES[];

#endif /* COPS_AND_ROBBERS_H_ */
