/*
 * rfreceiver.cpp
 *
 *  Created on: 5 dec 2015
 *      Author: Linus
 */

#include <rfreceiver.h>

rfreceiver::rfreceiver(uint8_t inputpin,  void (*receiverInt)(void),  void (*timerInt)(void))
{
	rfRxChannel_state = 0u;
	rfRxChannel_newData = false;
	rfRxChannel_len = 0u;
	rfRxChannel_index = 0u;
	m_inputpin = inputpin;
	pinMode(m_inputpin, INPUT);

	enable = true;

	for (timeout = 0u;timeout<MAX_NR_TIMES;timeout++)
	{
		buf[timeout] = 0u;
	}
	timeout = 0u;
	len = 0u;
	index = 0u;
	storeEnable = false;
	_timerInt = timerInt;
	_timerExpired = false;
	rfRxChannel_proto.data = 0u;
	rfRxChannel_proto.framecnt = 0u;
	rfRxChannel_proto.modfreq = 0u;
	rfRxChannel_proto.protocol = 0u;
	rfRxChannel_proto.repeats = 0u;
	rfRxChannel_proto.timeout = 0u;
	attachInterrupt(m_inputpin, receiverInt, CHANGE);
}

void rfreceiver::newPulse(uint16_t *buffer, uint8_t len, uint8_t index)
{
	if (len > MIN_NUM_PULSES)
	{
		rfRxChannel_newData = true;
		rfRxChannel_len = len;
		rfRxChannel_index = index;
	}
}

void rfreceiver::Process(void)
{
	switch (rfRxChannel_state)
	{
	case RF_STATE_IDLE:
	{
		/* If known protocol and timeout is not 0 (0 means burst) */
		if (rfRxChannel_proto.protocol != IR_PROTO_UNKNOWN && rfRxChannel_proto.timeout != 0) {
			/* Send button release command on CAN */
//				rfTxMsg.Data[0] = CAN_MODULE_ENUM_PHYSICAL_IR_STATUS_RELEASED;
//				rfTxMsg.Data[1] = rfRxChannel_proto.protocol;
			/* Data content is kept from last transmit (pressed) */

//				StdCan_Put(&rfTxMsg);
		}
		rfRxChannel_state = RF_STATE_START_RECEIVE;
		break;
	}

	case RF_STATE_START_RECEIVE:
		ETS_INTR_LOCK();
		rfRxChannel_newData = false;
		ETS_INTR_UNLOCK();
		rfRxChannel_state = RF_STATE_RECEIVING;

		break;

	case RF_STATE_RECEIVING:
		if (rfRxChannel_newData == true) {
			ETS_INTR_LOCK();
			rfRxChannel_newData = true;
			ETS_INTR_UNLOCK();
			/* Let protocol driver parse and then send on CAN */
			uint8_t res2 = 2;//parseProtocol(buf, rfRxChannel_len, rfRxChannel_index, &rfRxChannel_proto);
			res2 = parseProtocol(buf, rfRxChannel_len, rfRxChannel_index, &rfRxChannel_proto);
			if (res2 == IR_OK && rfRxChannel_proto.protocol != IR_PROTO_UNKNOWN)
			{
				/* If timeout is 0, protocol is burst protocol */
				if (rfRxChannel_proto.timeout > 0)
				{
//						rfTxMsg.Data[0] = CAN_MODULE_ENUM_PHYSICAL_IR_STATUS_PRESSED;
					rfRxChannel_state = RF_STATE_START_PAUSE;
				}
				else
				{
//						rfTxMsg.Data[0] = CAN_MODULE_ENUM_PHYSICAL_IR_STATUS_BURST;
					rfRxChannel_state = RF_STATE_START_RECEIVE;
				}
//					rfTxMsg.Data[1] = rfRxChannel_proto.protocol;
//					rfTxMsg.Data[2] = (rfRxChannel_proto.data>>40)&0xff;
//					rfTxMsg.Data[3] = (rfRxChannel_proto.data>>32)&0xff;
//					rfTxMsg.Data[4] = (rfRxChannel_proto.data>>24)&0xff;
//					rfTxMsg.Data[5] = (rfRxChannel_proto.data>>16)&0xff;
//					rfTxMsg.Data[6] = (rfRxChannel_proto.data>>8)&0xff;
//					rfTxMsg.Data[7] = rfRxChannel_proto.data&0xff;

//					StdCan_Put(&rfTxMsg);
				Serial.println("Got parsed packet");
			}
			else if (rfRxChannel_proto.protocol == IR_PROTO_UNKNOWN)
			{
#if (RF_SEND_DEBUG==1)
				//send_debug(rfRxChannel_buf, rfRxChannel_len);
				//rfRxChannel_proto.timeout=300;
#endif
				rfRxChannel_state = RF_STATE_START_RECEIVE;
			}
#if (RF_SEND_DEBUG==1)
			else if (res2 == IR_SEND_DEBUG)
			{
				send_debug(rfRxChannel_buf, rfRxChannel_proto.data & 0xFFu, rfRxChannel_index);
				rfRxChannel_state = RF_STATE_START_RECEIVE;
			}
#endif
		}
		break;

	case RF_STATE_START_PAUSE:
		/* set a timer so we can send release button event when no new RF is arriving */
		//Timer_SetTimeout(RF_RX_REPEATE_TIMER, rfRxChannel_proto.timeout, TimerTypeOneShot, 0);
		_timer.initializeMs(rfRxChannel_proto.timeout, _timerInt).startOnce();
		rfRxChannel_state = RF_STATE_PAUSING;
		break;

	case RF_STATE_PAUSING:
		/* reset timer if new IR arrived */
		if (rfRxChannel_newData == true) {
			ETS_INTR_LOCK();
			rfRxChannel_newData = false;
			ETS_INTR_UNLOCK();

			Ir_Protocol_Data_t	protoDummy;
			//parseProtocol(buf, rfRxChannel_len, rfRxChannel_index, &protoDummy)
			if (2 == IR_OK) {
				if (protoDummy.protocol == rfRxChannel_proto.protocol) {
					/* re-set timer so we can send release button event when no new RF is arriving */
					_timer.initializeMs(rfRxChannel_proto.timeout, _timerInt).startOnce();
				}
			}
		}

		//if (Timer_Expired(RF_RX_REPEATE_TIMER)) {
		if (_timerExpired) {
			rfRxChannel_state = RF_STATE_IDLE;
		}
		break;
	default:
		break;
	}
}

void rfreceiver::timer_int(void)
{
	_timerExpired = true;
}
void rfreceiver::rx_int(void)
{
	static uint32 prev_time;
		uint16 pulsewidth;

		/* Read the timer counter register to get the current "time". */
		uint32 time = micros();

		/* Subtract the current measurement from the previous to get the pulse width. */
		pulsewidth = (uint16)(time - prev_time);
		prev_time = time;

	/* in continuous mode when short pulse arrives received len should be set to zero */
	//TODO?
		if ((pulsewidth < (IR_MIN_PULSE_WIDTH)) && (storeEnable == true))
		{
			len = 0;
			return;
		}
		else
		{
		}

		if (storeEnable)
		{
			/* Store the measurement. */
			buf[index++] = pulsewidth;

			if (index == MAX_NR_TIMES)
			{
				index = 0;
			}
			if (len++ == MAX_NR_TIMES)
			{
				len = MAX_NR_TIMES-1;
			}
			/* Notify the application that a pulse has been received. */
			this->newPulse(buf, len, index);
		}
		else if (enable == true)
		{
			/* The first edge of the pulse train has been detected. Enable the storage of the following pulsewidths. */
			storeEnable = true;
			len = 0;
		}
}

