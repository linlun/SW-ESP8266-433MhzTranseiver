/*
 * rfreceiver.h
 *
 *  Created on: 5 dec 2015
 *      Author: Linus
 */

#ifndef APP_RFRECEIVER_H_
#define APP_ROOMBA_H_

#include <config.h>
#include <stdlib.h>
#include <protocols.h>
#include <SmingCore/SmingCore.h>

#define RF_SEND_DEBUG	0



#define RF_STATE_IDLE						(0)
#define RF_STATE_IR_REPEAT				(1)
#define RF_STATE_START_RECEIVE			(2)
#define RF_STATE_RECEIVING				(3)
#define RF_STATE_START_PAUSE				(4)
#define RF_STATE_PAUSING					(5)
#define RF_STATE_START_IDLE				(6)

#define RF_STATE_START_TRANSMIT			(7)

#define RF_STATE_TRANSMITTING				(11)

#define RF_STATE_DISABLED			0xff

#define NR_BUFFERS					16
#define MAX_ITERATIONS_PER_CALL 	15

/* Protocol data structure */
typedef struct {
	sint16 startPtr;
	sint16 writePtr;
	sint16 parsedPtr;
	uint16 lenght;
	bool active;
} Ir_Buffer_Data_t;


class rfreceiver {
  public:
	rfreceiver(uint8_t inputpin);
	void Process(void);
	void rx_int(void);
	void start(int processPeriod);
	Ir_Buffer_Data_t 	buffers[NR_BUFFERS];
	sint16		buf_index;
	uint8_t	writeBufferPtr;
	uint8_t	readBufferPtr;
  private:
	void newPulse(uint16_t *buffer, uint8_t len, uint8_t index);
	uint8_t m_inputpin;
	uint8_t		enable;
	uint16_t	timeout;
	uint16_t	buf[MAX_NR_TIMES];
	sint16		OldestBufferItem;
	uint16_t		len;

	uint8_t		storeEnable;
	uint8_t rfRxChannel_newData;
	uint8_t rfRxChannel_len;
	uint8_t rfRxChannel_index;
	uint8_t rfRxChannel_state;
	Ir_Protocol_Data_t	rfRxChannel_proto;

	bool _timerExpired;
	Timer _timer;
};

#endif /* APP_RFRECEIVER_H_ */
