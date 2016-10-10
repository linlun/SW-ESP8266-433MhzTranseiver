/*
 * rfreceiver.cpp
 *
 *  Created on: 5 dec 2015
 *      Author: Linus
 */

#include <rfreceiver.h>

#define INCREMENT(nr, maxValue) ((nr)==((maxValue)-1)?(0):((nr)+1))
#define DECREMENT(nr, maxValue) ((nr)==(0)?((maxValue)-1):((nr)-1))
#define INDEX_ADD(value, add, max)	((((value) + (add))<=(max))?((value) + (add)):((value) + (add) - (max)))
#define INDEX_SUBTRACT(value, sub, max)	((((value) - (sub))> 0)?((value) - (sub)):((value) - (sub) + (max)))


void IRAM_ATTR rfreceiver::rx_int(void)
{
	static uint32 prev_time;
	uint16 pulsewidth;

	/* Read the timer counter register to get the current "time". */
	uint32 time = micros();

	/* Subtract the current measurement from the previous to get the pulse width. */
	pulsewidth = (uint16)(time - prev_time);
	prev_time = time;

	if (((pulsewidth > (IR_MAX_PULSE_WIDTH)) || (pulsewidth < (IR_MIN_PULSE_WIDTH))))
	{
		if (buffers[writeBufferPtr].lenght > MIN_NUM_PULSES)
		{
			//Remove this when not debugging
			OldestBufferItem = buffers[writeBufferPtr].startPtr;

			//Serial.print(".");
			//uint16 oldPtr = buffers[writeBufferPtr].writePtr;
			buffers[writeBufferPtr].active = false;
			/* We have a complete buffer of data */
			writeBufferPtr = INCREMENT(writeBufferPtr, NR_BUFFERS);

			//Add lines below when not debugging
			//if (writeBufferPtr == readBufferPtr)
			//{
			//	/* all buffers used, overwrite data in last buffer */
			//	writeBufferPtr = DECREMENT(writeBufferPtr, NR_BUFFERS);
			//}

			buffers[writeBufferPtr].startPtr = buf_index;
		}
		buffers[writeBufferPtr].active = true;
		buf_index = buffers[writeBufferPtr].startPtr;
		buffers[writeBufferPtr].lenght = 0u;
		buffers[writeBufferPtr].parsedPtr = buffers[writeBufferPtr].startPtr;
		buffers[writeBufferPtr].writePtr = DECREMENT(buffers[writeBufferPtr].startPtr, MAX_NR_TIMES);
		//index = buffers[writeBufferPtr].startPtr;
		return;
	}
	else
	{
		buf[buf_index] = pulsewidth;
		buffers[writeBufferPtr].lenght++;
		buffers[writeBufferPtr].writePtr = buf_index;
		buf_index = INCREMENT(buf_index, MAX_NR_TIMES);
		/*index++;
		//Serial.print("*");
		//Serial.print(" ");
		//	Serial.print(index);

		if (index > MAX_NR_TIMES)
		{
			index = 0u;
		}
		*/
		/* Check if buffer overflow, in that case, rewrite current buffer */
		if (buf_index == OldestBufferItem)
		{
			buf_index = buffers[writeBufferPtr].startPtr;
			//buffers[writeBufferPtr].active = true;
			//buffers[writeBufferPtr].startPtr = index;
			buffers[writeBufferPtr].lenght = 0u;
			buffers[writeBufferPtr].parsedPtr = buffers[writeBufferPtr].startPtr;
			buffers[writeBufferPtr].writePtr = DECREMENT(buffers[writeBufferPtr].startPtr, MAX_NR_TIMES);
		}
	}
}

rfreceiver::rfreceiver(uint8_t inputpin)
{
	rfRxChannel_state = RF_STATE_RECEIVING;
	rfRxChannel_newData = false;
	rfRxChannel_len = 0u;
	rfRxChannel_index = 0u;
	m_inputpin = inputpin;
	pinMode(m_inputpin, INPUT);

	writeBufferPtr = 0u;
	readBufferPtr = 0u;
	OldestBufferItem = 0u;
	enable = true;
	for (timeout = 0u;timeout<NR_BUFFERS;timeout++)
	{
		buffers[timeout].active = false;
		buffers[timeout].lenght = 0u;
		buffers[timeout].parsedPtr = 0u;
		buffers[timeout].startPtr = 0u;
		buffers[timeout].writePtr = 0u;
	}
	buffers[0].active = true;
	for (timeout = 0u;timeout<MAX_NR_TIMES;timeout++)
	{
		buf[timeout] = 0u;
	}
	timeout = 0u;
	len = 0u;
	buf_index = 0u;
	storeEnable = false;
	_timerExpired = false;
	rfRxChannel_proto.data = 0u;
	rfRxChannel_proto.framecnt = 0u;
	rfRxChannel_proto.modfreq = 0u;
	rfRxChannel_proto.protocol = 0u;
	rfRxChannel_proto.repeats = 0u;
	rfRxChannel_proto.timeout = 0u;

}

void rfreceiver::start(int processPeriod)
{
	_timer.initializeMs(processPeriod , TimerDelegate(&rfreceiver::Process,this)).start();
	attachInterrupt(m_inputpin, Delegate<void()>(&rfreceiver::rx_int, this), CHANGE);
}

#define INDEX_DIFF(start, current, max)	(((start) < (current))?((current) - (start)):(((current) + (max)) - (start)))

void rfreceiver::Process(void)
{
	/*
	static sint16 old = 0;
	static sint16 cntold = 0;
	if (buffers[writeBufferPtr].lenght != old)
	{
		cntold++;
		if (cntold >= 2)
		{
			cntold = 0;
			Serial.print(buffers[writeBufferPtr].lenght);
			Serial.print(" ");
			Serial.print(writeBufferPtr);
			Serial.print(" ");
			Serial.print(readBufferPtr);
			Serial.print(" ");
			Serial.print(buffers[readBufferPtr].parsedPtr);
			//Serial.print(" ");
			//Serial.print((uint8)((rfRxChannel_proto.data>>40)&0xff),16);

		}
		old = buffers[writeBufferPtr].lenght;
	}
	Serial.print(" ");
	Serial.print(buf_index);
	Serial.print(" ");
	Serial.print(buffers[writeBufferPtr].lenght);
	Serial.print(" ");
	Serial.print(writeBufferPtr);
	Serial.print(" ");
	Serial.print(readBufferPtr);
	Serial.print(" ");
	Serial.println(buffers[readBufferPtr].parsedPtr);
*/
	if (buffers[readBufferPtr].active == true)
	{
		//Wait for current buffer to be done
		return;
	}
	uint8 MaxNumberOfIterationsPerCall = MAX_ITERATIONS_PER_CALL;
	if (buffers[readBufferPtr].lenght < MIN_NUM_PULSES)
	{
		// No more useful data in buffer
		readBufferPtr = INCREMENT(readBufferPtr, NR_BUFFERS);
		OldestBufferItem = buffers[readBufferPtr].startPtr;
		return;
	}
	else
	{
		//Make sure parsedPtr points to a location that is at least MIN_NUM_PULSES from startPtr
		//if (INDEX_DIFF(buffers[readBufferPtr].startPtr, buffers[readBufferPtr].parsedPtr, MAX_NR_TIMES) < MIN_NUM_PULSES)
		//{
		//	buffers[readBufferPtr].parsedPtr = INDEX_ADD(buffers[readBufferPtr].startPtr, MIN_NUM_PULSES, MAX_NR_TIMES);
		//}
		while (buffers[readBufferPtr].lenght >= MIN_NUM_PULSES)
		{
			MaxNumberOfIterationsPerCall--;
			uint16 used_length = 0u;
			uint8 res = parseProtocol(buf, buffers[readBufferPtr].lenght, buffers[readBufferPtr].startPtr, &rfRxChannel_proto,&used_length);
			if (res == IR_OK && rfRxChannel_proto.protocol != IR_PROTO_UNKNOWN)
			{
				//Found data
				// Send/store response
				Serial.print("Got data: ");
				Serial.print(rfRxChannel_proto.protocol);
				Serial.print(" ");
				Serial.print((uint8)((rfRxChannel_proto.data>>40)&0xff),16);
				Serial.print(" ");
				Serial.print((uint8)((rfRxChannel_proto.data>>32)&0xff),16);
				Serial.print(" ");
				Serial.print((uint8)((rfRxChannel_proto.data>>24)&0xff),16);
				Serial.print(" ");
				Serial.print((uint8)((rfRxChannel_proto.data>>16)&0xff),16);
				Serial.print(" ");
				Serial.print((uint8)((rfRxChannel_proto.data>>8)&0xff),16);
				Serial.print(" ");
				Serial.println((uint8)(rfRxChannel_proto.data&0xff),16);
				Serial.print(" ");
				Serial.println((unsigned long)rfRxChannel_proto.data);
				buffers[readBufferPtr].lenght -= used_length;
				buffers[readBufferPtr].startPtr = INDEX_ADD(buffers[readBufferPtr].startPtr, used_length, MAX_NR_TIMES);
				OldestBufferItem = buffers[readBufferPtr].startPtr;
				return;
			}
#if (IR_PROTOCOLS_USE_RUBICSONSTN)
			else if (res == IR_OK_LONG)
			{
/*
				Protokollet verkar vara såhär:
14 bytes skickas ut från sensorn (ca 45 sekunders mellanrum), som jag har tolkat det så är dessa bytes uppdelade på detta vis (varje tecken motsvarar en bit):
ssssssss iiiiiiii iiiiiiii 0000000b ddd0tttt tttttttt hhhhhhhh aaaaaaaa mmmmmmmm rrrrrrrr rrrrrrrr rrrrrrrr cccccccc cccccccc
s = Startbyte, alltid 0xF5
i = Id, dessa två byte verkar slumpmässigt valda varje gång man byter batteri
b = Flagga för lågt batteri, 0=OK, 1=Low bat
d = Tre bitar med vindriktning (8 riktningar, 0 = Nord)
t = Temperatur, kodat i 1/10 grader, Ex: 0x104 = 260 = 26.0 grader, har ej testat minusgrader, men gissar på att det skrivs som 2-komplementet.
h = Luftfuktighet, Ex: 0x23 = 35 = 35%
a = Medelvindhastighet, Ex: 0x06 = 6 = 6 * 0.8 = 4.8m/s
m = Max vindhastighet, Ex: 0x06 = 6 = 6 * 0.8 = 4.8m/s
r = Regnmängd, gissar på att inte alla byte används för detta, möjligt att någon medelregnmängd finns med här. Men iaf lägsta delen är antalet vipp som sensorn gjort sendan start, varje vipp motsvarar 0.4mm regn.
c = CRC, Vet ej hur denna räknas ut.
*/
				//printf("d %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", receiverBufferLong[0],receiverBufferLong[1],receiverBufferLong[2],receiverBufferLong[3],receiverBufferLong[4],receiverBufferLong[5],receiverBufferLong[6],receiverBufferLong[7],receiverBufferLong[8],receiverBufferLong[9],receiverBufferLong[10],receiverBufferLong[11],receiverBufferLong[12],receiverBufferLong[13]);

				//TODO: This data packageing shall be done in the the protocol decoding and not here
				receiverBufferLong[0] = receiverBufferLong[1] ^ receiverBufferLong[2];
				receiverBufferLong[1] = (receiverBufferLong[4] & 0x0Fu) + ((receiverBufferLong[4] & 0xE0u)>>1);
				receiverBufferLong[2] = receiverBufferLong[5];
				receiverBufferLong[4] = receiverBufferLong[7];
				receiverBufferLong[5] = receiverBufferLong[8];

				receiverBufferLong[8] = receiverBufferLong[3];
				receiverBufferLong[3] = receiverBufferLong[6];

				receiverBufferLong[6] = receiverBufferLong[0];
				receiverBufferLong[7] = 0x80; //Second packet
				receiverBufferLong[9] = receiverBufferLong[9];
				receiverBufferLong[10] = receiverBufferLong[10];
				receiverBufferLong[11] = receiverBufferLong[11];

				// Here only 12 bytes in order shall be put into the two CAN frames.
				/*
				rfRxChannel_state = sns_rfTransceive_STATE_START_RECEIVE;
				rfTxMsg.Data[0] = CAN_MODULE_ENUM_PHYSICAL_IR_STATUS_BURST;
				rfTxMsg.Data[1] = rfRxChannel_proto.protocol;
				rfTxMsg.Data[2] = receiverBufferLong[0];
				rfTxMsg.Data[3] = receiverBufferLong[1];
				rfTxMsg.Data[4] = receiverBufferLong[2];
				rfTxMsg.Data[5] = receiverBufferLong[3];
				rfTxMsg.Data[6] = receiverBufferLong[4];
				rfTxMsg.Data[7] = receiverBufferLong[5];
				StdCan_Put(&rfTxMsg);
				rfTxMsg.Data[0] = CAN_MODULE_ENUM_PHYSICAL_IR_STATUS_BURST;
				rfTxMsg.Data[1] = rfRxChannel_proto.protocol;
				rfTxMsg.Data[2] = receiverBufferLong[6];
				rfTxMsg.Data[3] = receiverBufferLong[7];
				rfTxMsg.Data[4] = receiverBufferLong[8];
				rfTxMsg.Data[5] = receiverBufferLong[9];
				rfTxMsg.Data[6] = receiverBufferLong[10];
				rfTxMsg.Data[7] = receiverBufferLong[11];
				StdCan_Put(&rfTxMsg);
				*/
				// Send/store response
				Serial.print("Got data: ");
				Serial.print(rfRxChannel_proto.protocol);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[0]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[1]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[2]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[3]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[4]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[5]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[6]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[7]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[8]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[9]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[10]),16);
				Serial.print(" ");
				Serial.print((uint8)(receiverBufferLong[11]),16);
				Serial.println("");
				buffers[readBufferPtr].lenght -= used_length;
				buffers[readBufferPtr].startPtr = INDEX_ADD(buffers[readBufferPtr].startPtr, used_length, MAX_NR_TIMES);
				OldestBufferItem = buffers[readBufferPtr].startPtr;
				return;
			}

#endif
			// Check if more data exists in buffer
			if (buffers[readBufferPtr].lenght < MIN_NUM_PULSES)
			{
				// Set next buffer as active for read
				readBufferPtr = INCREMENT(readBufferPtr, NR_BUFFERS);
				OldestBufferItem = buffers[readBufferPtr].startPtr;
				return;
			}
			buffers[readBufferPtr].startPtr = INCREMENT(buffers[readBufferPtr].startPtr, MAX_NR_TIMES);
			buffers[readBufferPtr].lenght--;
			if (MaxNumberOfIterationsPerCall == 0u)
			{
				return;
			}
		}
	}
}


//void rfreceiver::Process(void)
//{
//	switch (rfRxChannel_state)
//	{
//	case RF_STATE_IDLE:
//	{
//		/* If known protocol and timeout is not 0 (0 means burst) */
//		if (rfRxChannel_proto.protocol != IR_PROTO_UNKNOWN && rfRxChannel_proto.timeout != 0) {
//			/* Send button release command on CAN */
////				rfTxMsg.Data[0] = CAN_MODULE_ENUM_PHYSICAL_IR_STATUS_RELEASED;
////				rfTxMsg.Data[1] = rfRxChannel_proto.protocol;
//			/* Data content is kept from last transmit (pressed) */
//
////				StdCan_Put(&rfTxMsg);
//		}
//		rfRxChannel_state = RF_STATE_START_RECEIVE;
//		break;
//	}
//
//	case RF_STATE_START_RECEIVE:
//		ETS_INTR_LOCK();
//		rfRxChannel_newData = false;
//		ETS_INTR_UNLOCK();
//		rfRxChannel_state = RF_STATE_RECEIVING;
//
//		break;
//
//	case RF_STATE_RECEIVING:
//		if (rfRxChannel_newData == true) {
//			ETS_INTR_LOCK();
//			rfRxChannel_newData = false;
//			ETS_INTR_UNLOCK();
//			//Serial.println("Got new data");
//			/* Let protocol driver parse and then send on CAN */
//			uint8_t res2 = 2;//parseProtocol(buf, rfRxChannel_len, rfRxChannel_index, &rfRxChannel_proto);
//			res2 = parseProtocol(buf, rfRxChannel_len, rfRxChannel_index, &rfRxChannel_proto);
//			if (res2 == IR_OK && rfRxChannel_proto.protocol != IR_PROTO_UNKNOWN)
//			{
//				/* If timeout is 0, protocol is burst protocol */
//				if (rfRxChannel_proto.timeout > 0)
//				{
////						rfTxMsg.Data[0] = CAN_MODULE_ENUM_PHYSICAL_IR_STATUS_PRESSED;
//					rfRxChannel_state = RF_STATE_START_PAUSE;
//				}
//				else
//				{
////						rfTxMsg.Data[0] = CAN_MODULE_ENUM_PHYSICAL_IR_STATUS_BURST;
//					rfRxChannel_state = RF_STATE_START_RECEIVE;
//				}
////					rfTxMsg.Data[1] = rfRxChannel_proto.protocol;
////					rfTxMsg.Data[2] = (rfRxChannel_proto.data>>40)&0xff;
////					rfTxMsg.Data[3] = (rfRxChannel_proto.data>>32)&0xff;
////					rfTxMsg.Data[4] = (rfRxChannel_proto.data>>24)&0xff;
////					rfTxMsg.Data[5] = (rfRxChannel_proto.data>>16)&0xff;
////					rfTxMsg.Data[6] = (rfRxChannel_proto.data>>8)&0xff;
////					rfTxMsg.Data[7] = rfRxChannel_proto.data&0xff;
//					Serial.print("Got data: ");
//					Serial.print(rfRxChannel_proto.protocol);
//					Serial.print(" ");
//					Serial.print((uint8)((rfRxChannel_proto.data>>40)&0xff),16);
//					Serial.print((uint8)((rfRxChannel_proto.data>>32)&0xff),16);
//					Serial.print((uint8)((rfRxChannel_proto.data>>24)&0xff),16);
//					Serial.print((uint8)((rfRxChannel_proto.data>>16)&0xff),16);
//					Serial.print((uint8)((rfRxChannel_proto.data>>8)&0xff),16);
//					Serial.println((uint8)(rfRxChannel_proto.data&0xff),16);
////					StdCan_Put(&rfTxMsg);
//			}
//			else if (rfRxChannel_proto.protocol == IR_PROTO_UNKNOWN)
//			{
//#if (RF_SEND_DEBUG==1)
//				//send_debug(rfRxChannel_buf, rfRxChannel_len);
//				//rfRxChannel_proto.timeout=300;
//#endif
//				//rfRxChannel_state = RF_STATE_START_RECEIVE;
//				//Serial.println("Did not find proto");
///*
//				while (rfRxChannel_len != 0)
//				{
//					Serial.print(" ");
//					Serial.print(buf[rfRxChannel_len]);
//					rfRxChannel_len--;
//				}
//			*/
//			}
//#if (RF_SEND_DEBUG==1)
//			else if (res2 == IR_SEND_DEBUG)
//			{
//				send_debug(rfRxChannel_buf, rfRxChannel_proto.data & 0xFFu, rfRxChannel_index);
//				rfRxChannel_state = RF_STATE_START_RECEIVE;
//			}
//#endif
//		}
//		break;
//
//	case RF_STATE_START_PAUSE:
//		/* set a timer so we can send release button event when no new RF is arriving */
//		//Timer_SetTimeout(RF_RX_REPEATE_TIMER, rfRxChannel_proto.timeout, TimerTypeOneShot, 0);
//		_timer.initializeMs(rfRxChannel_proto.timeout, _timerInt).startOnce();
//		rfRxChannel_state = RF_STATE_PAUSING;
//		break;
//
//	case RF_STATE_PAUSING:
//		/* reset timer if new IR arrived */
//		if (rfRxChannel_newData == true) {
//			ETS_INTR_LOCK();
//			rfRxChannel_newData = false;
//			ETS_INTR_UNLOCK();
//
//			Ir_Protocol_Data_t	protoDummy;
//			//parseProtocol(buf, rfRxChannel_len, rfRxChannel_index, &protoDummy)
//			if (parseProtocol(buf, rfRxChannel_len, rfRxChannel_index, &protoDummy) == IR_OK) {
//				if (protoDummy.protocol == rfRxChannel_proto.protocol) {
//					/* re-set timer so we can send release button event when no new RF is arriving */
//					_timer.initializeMs(rfRxChannel_proto.timeout, _timerInt).startOnce();
//				}
//			}
//		}
//
//		//if (Timer_Expired(RF_RX_REPEATE_TIMER)) {
//		if (_timerExpired) {
//			rfRxChannel_state = RF_STATE_IDLE;
//		}
//		break;
//	default:
//		break;
//	}
//}

