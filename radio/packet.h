/*
 * packet.h
 *
 *  Created on: 26-Apr-2009
 *	Updated on: 26-Jan
 *      Author: Neil MacMillan, Daniel McIlvaney
 */

#ifndef PACKET_H_
#define PACKET_H_


#include <avr/io.h>

#include "../cops_and_robbers.h"

/*****					Add labels for the packet types to the enumeration					*****/

typedef enum _pt
{
	GAMESTATE_PACKET,		/// a gamestate update from the base station to the Roomba
	ROOMBASTATE_PACKET,		/// a reply from the Roomba, only sent to report a death or revive.
	MESSAGE_PACKET			/// a generic message (not implemented by the Roomba)
} PACKET_TYPE;

/*****							Construct payload format structures							*****/
//								structures must be 29 bytes long or less.

/// Extranious unused packet from the original provided packet structure.
typedef struct _msg
{
	uint8_t address[5];
	uint8_t messagecontent[24];
} pf_message_t;

/*****							Add format structures to the union							*****/

/// The application-dependent packet format.  Add structures to the union that correspond to the packet types defined
/// in the PACKET_TYPE enumeration.  The format structures may not be more than 29 bytes long.  The _filler array must
/// be included to ensure that the union is exactly 29 bytes long.
typedef union _pf
{
	uint8_t _filler[29];	// makes sure the packet is exactly 32 bytes long - this array should not be accessed directly.
	pf_gamestate_t gamestate;
	pf_roombastate_t roombastate;
	pf_message_t message;
} payloadformat_t;

/*****						Leave the radiopacket_t structure alone.						*****/

typedef struct _rp
{
	uint8_t type;
	uint16_t timestamp;
	payloadformat_t payload;
} radiopacket_t;

#endif /* PACKET_H_ */
