/*
  RCSwitch - Arduino libary for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.

  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
  - Dominik Fischer / dom_fischer(at)web(dot)de
  - Frank Oltmanns / <first name>.<last name>(at)gmail(dot)com
  
  Project home: http://code.google.com/p/rc-switch/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef _RCSwitch_h
#define _RCSwitch_h

#include "SmingCore.h"

// Number of maximum High/Low changes per packet.
// We can handle up to (unsigned long) => 32 bit * 2 H/L changes per bit + 2 for sync
#define RCSWITCH_MAX_CHANGES 250

#define PROTOCOL3_SYNC_FACTOR   71
#define PROTOCOL3_0_HIGH_CYCLES  4
#define PROTOCOL3_0_LOW_CYCLES  11
#define PROTOCOL3_1_HIGH_CYCLES  9
#define PROTOCOL3_1_LOW_CYCLES   6

#define IR_PROTOCOL_RC5 0
#define IR_PROTOCOL_RC6 1
#define IR_PROTOCOL_RCMM 2
#define IR_PROTOCOL_SIRC 3
#define IR_PROTOCOL_SHARP 4
#define IR_PROTOCOL_NEC 5
#define IR_PROTOCOL_SAMSUNG 6
#define IR_PROTOCOL_MARANTZ 7
#define IR_PROTOCOL_PANASONIC 8
#define IR_PROTOCOL_SKY 9
#define IR_PROTOCOL_NEXA2 10
#define IR_PROTOCOL_NEXA 11
#define IR_PROTOCOL_VIKING_ 12
#define IR_PROTOCOL_VIKINGSTEAK 	13
#define IR_PROTOCOL_RUBICSON 	14
#define IR_PROTOCOL_IROBOT 15
#define IR_PROTOCOL_OREGONRAIN 16
#define IR_PROTOCOL_OREGONTEMPHUM 17
#define IR_PROTOCOL_OREGONWIND 18
#define IR_PROTOCOL_RUBICSONSTN 19
#define IR_PROTOCOL_UNKNOWN 255

/* Nexa2 Implementation
 * Receiver: DONE
 * Transmitter: DONE
 */
#define IR_NEXA2_START1 	(10000)	//us
#define IR_NEXA2_START2 	(2500)	//us
#define IR_NEXA2_HIGH 		(210)		//us
#define IR_NEXA2_LOW_ONE	(320)		//us
#define IR_NEXA2_LOW_ZERO	(1200)	//us
#define IR_NEXA2_TIMEOUT	(200)								//ms	(time between ir frames)
#define IR_NEXA2_REPS		(5)									//		(minimum number of times to repeat code)
#define IR_NEXA2_F_MOD		(38)								//kHz	(modulation frequency)
#define IR_NEXA2_TOL_DIV	(2)

/* Rubicson weather station Implementation
 * Receiver: 
 */
#define IR_RUBICSONSTN_HIGH		(500)		//us
#define IR_RUBICSONSTN_LOW_ONE		(1000)	//us
#define IR_RUBICSONSTN_LOW_ZERO		(500)		//us
#define IR_RUBICSONSTN_LOW_START	(2400)	//us
#define IR_RUBICSONSTN_TIMEOUT		(200)					//ms BURST!	(time between ir frames)
#define IR_RUBICSONSTN_REPS		(1)					//		(minimum number of times to repeat code)
#define IR_RUBICSONSTN_F_MOD		(38)					//kHz	(modulation frequency)
#define IR_RUBICSONSTN_TOL_DIV		(4)

/* Rubicson temperature sensor Implementation
 * Receiver:
 */
#define IR_RUBICSON_HIGH		(550)		//us
#define IR_RUBICSON_LOW_ONE		(1880)		//us
#define IR_RUBICSON_LOW_ZERO		(900)	//us
#define IR_RUBICSON_LOW_START		(3850)		//us
#define IR_RUBICSON_TIMEOUT		(200)									//ms BURST!	(time between ir frames)
#define IR_RUBICSON_REPS		(1)									//		(minimum number of times to repeat code)
#define IR_RUBICSON_F_MOD		(38)								//kHz	(modulation frequency)
#define IR_RUBICSON_TOL_DIV		(4)

/* Viking Steak temperature sensor Implementation
 * Receiver:
 */
#define IR_VIKING_STEAK_HIGH		(700)		//us
#define IR_VIKING_STEAK_LOW_ONE	(4000)		//us
#define IR_VIKING_STEAK_LOW_ZERO	(1700)	//us
#define IR_VIKING_STEAK_LOW_START	(7800)		//us
#define IR_VIKING_STEAK_TIMEOUT	(200)									//ms BURST!	(time between ir frames)
#define IR_VIKING_STEAK_REPS		(1)									//		(minimum number of times to repeat code)
#define IR_VIKING_STEAK_F_MOD		(38)								//kHz	(modulation frequency)
#define IR_VIKING_STEAK_TOL_DIV	(4)


class RCSwitch {

  public:
    RCSwitch();
    
    #if not defined( RCSwitchDisableReceiving )
    void enableReceive(int interrupt);
    void enableReceive();
    void disableReceive();
    bool available();
    void resetAvailable();
	
    unsigned long long getReceivedValue();
    unsigned int getReceivedBitlength();
    unsigned int getReceivedDelay();
    unsigned int getReceivedProtocol();
    unsigned int* getReceivedRawdata();
	uint8* getReceivedLongdata();
	bool getReceivedJSON(JsonObject& json);
    #endif
  
    void enableTransmit(int nTransmitterPin);
    void disableTransmit();

  private:
    #if not defined( RCSwitchDisableReceiving )
    static void handleInterrupt();
    static bool receiveProtocol1(unsigned int changeCount);
    static bool receiveProtocol2(unsigned int changeCount);
    static bool receiveProtocol3(unsigned int changeCount);
	
    static bool receiveProtocolNexa2(unsigned int changeCount);
	static bool receiveProtocolRubicsonStation(unsigned int changeCount);
	static bool receiveProtocolRubicsonTemperature(unsigned int changeCount);
	static bool receiveProtocolVikingSteak(unsigned int changeCount);
	static uint8 receiverBufferLong[14];
    int nReceiverInterrupt;
    #endif
    int nTransmitterPin;
    #if not defined( RCSwitchDisableReceiving )
    static int nReceiveTolerance;
    static unsigned long long nReceivedValue;
    static unsigned int nReceivedBitlength;
    static unsigned int nReceivedDelay;
    static unsigned int nReceivedProtocol;
    #endif
    /* 
     * timings[0] contains sync timing, followed by a number of bits
     */
    static unsigned int timings[RCSWITCH_MAX_CHANGES];
};

#endif
