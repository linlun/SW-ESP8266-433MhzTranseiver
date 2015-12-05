/*
 * rfreceiver.h
 *
 *  Created on: 5 dec 2015
 *      Author: Linus
 */

#ifndef APP_RFRECEIVER_H_
#define APP_RFRECEIVER_H_

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

class rfreceiver {
  public:
	rfreceiver(uint8_t inputpin,  void (*receiverInt)(void),  void (*timerInt)(void));
	void Process(void);
	void rx_int(void);
	void timer_int(void);

  private:
	void newPulse(uint16_t *buffer, uint8_t len, uint8_t index);
	uint8_t m_inputpin;
	uint8_t		enable;
	uint16_t	timeout;
	uint16_t	buf[MAX_NR_TIMES];
	uint8_t		len;
	uint8_t		index;
	uint8_t		storeEnable;
	uint8_t rfRxChannel_newData;
	uint8_t rfRxChannel_len;
	uint8_t rfRxChannel_index;
	uint8_t rfRxChannel_state;
	Ir_Protocol_Data_t	rfRxChannel_proto;
	bool _timerExpired;
	Timer _timer;
	void (*_timerInt)(void);
};

#endif /* APP_RFRECEIVER_H_ */
