/*
  RCSwitch - Arduino libary for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.
  
  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
  - Dominik Fischer / dom_fischer(at)web(dot)de
  - Frank Oltmanns / <first name>.<last name>(at)gmail(dot)com
  - Andreas Steinel / A.<lastname>(at)gmail(dot)com
  
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

#include "RCSwitch.h"
#include <math.h>
#if not defined( RCSwitchDisableReceiving )
unsigned long long RCSwitch::nReceivedValue = 0;
unsigned int RCSwitch::nReceivedBitlength = 0;
unsigned int RCSwitch::nReceivedDelay = 0;
unsigned int RCSwitch::nReceivedProtocol = 0;
#endif
unsigned int RCSwitch::timings[RCSWITCH_MAX_CHANGES];
uint8 RCSwitch::receiverBufferLong[14];

RCSwitch::RCSwitch() {
  this->nTransmitterPin = -1;
   #if not defined( RCSwitchDisableReceiving )
  this->nReceiverInterrupt = -1;
  RCSwitch::nReceivedValue = 0;
  #endif
}

/**
 * Enable transmissions
 *
 * @param nTransmitterPin    Arduino Pin to which the sender is connected to
 */
void RCSwitch::enableTransmit(int nTransmitterPin) {
  this->nTransmitterPin = nTransmitterPin;
  pinMode(this->nTransmitterPin, OUTPUT);
}

/**
  * Disable transmissions
  */
void RCSwitch::disableTransmit() {
  this->nTransmitterPin = -1;
}

#if not defined( RCSwitchDisableReceiving )
/**
 * Enable receiving data
 */
void RCSwitch::enableReceive(int interrupt) {
  this->nReceiverInterrupt = interrupt;
  this->enableReceive();
}

void RCSwitch::enableReceive() {
  if (this->nReceiverInterrupt != -1) {
    RCSwitch::nReceivedValue = 0;
    RCSwitch::nReceivedBitlength = 0;
    attachInterrupt(this->nReceiverInterrupt, handleInterrupt, CHANGE);
  }
}

/**
 * Disable receiving data
 */
void RCSwitch::disableReceive() {
  detachInterrupt(this->nReceiverInterrupt);
  this->nReceiverInterrupt = -1;
}

bool RCSwitch::available() {
  return RCSwitch::nReceivedValue != 0;
}

void RCSwitch::resetAvailable() {
  RCSwitch::nReceivedValue = 0;
}

unsigned long long RCSwitch::getReceivedValue() {
    return RCSwitch::nReceivedValue;
}

unsigned int RCSwitch::getReceivedBitlength() {
  return RCSwitch::nReceivedBitlength;
}

unsigned int RCSwitch::getReceivedDelay() {
  return RCSwitch::nReceivedDelay;
}

unsigned int RCSwitch::getReceivedProtocol() {
  return RCSwitch::nReceivedProtocol;
}

unsigned int* RCSwitch::getReceivedRawdata() {
    return RCSwitch::timings;
}

uint8* RCSwitch::getReceivedLongdata() {
    return RCSwitch::receiverBufferLong;
}

bool RCSwitch::getReceivedJSON(JsonObject& json)
{
	float windspeed_avg;
	float windspeed_max;
	uint8 humidity;
	sint16 temp;
	float winddirection;
	uint8 addr;
	uint8 sign;
	float temp2;
	uint8 battery;
	float rain;
	uint8 crc;
	uint8 byte0;
	uint8 byte1;
	uint8 byte2;
	switch (this->nReceivedProtocol)
	{
	case IR_PROTOCOL_NEXA2:

		json["id"] = (unsigned long)(nReceivedValue&0x3FFFFFF); //26 bit id
		json["group"] = (uint8)((((nReceivedValue>>26)&0x1) == 1)? 0u:1u);
		json["switch"] = (uint8)((((nReceivedValue>>27)&0x1) == 1)? 0u:1u);
		byte0 = (uint8)((nReceivedValue>>28)&0xF);
		if (byte0 & 1)
			byte0 ^= 0xf;
		json["button"] = byte0;
		byte0 = (uint8)((nReceivedValue>>32)&0xFF);
		switch (byte0)
		{
		case 15:
			byte0 = 1;
			break;
		case 7:
			byte0 = 2;
			break;
		case 11:
			byte0 = 3;
			break;
		case 3:
			byte0 = 4;
			break;
		case 13:
			byte0 = 5;
			break;
		case 5:
			byte0 = 6;
			break;
		case 9:
			byte0 = 7;
			break;
		case 1:
			byte0 = 8;
			break;
		case 14:
			byte0 = 9;
			break;
		case 6:
			byte0 = 10;
			break;
		case 10:
			byte0 = 11;
			break;
		case 2:
			byte0 = 12;
			break;
		case 12:
			byte0 = 13;
			break;
		case 4:
			byte0 = 14;
			break;
		case 8:
			byte0 = 15;
			break;
		default:
			break;
		}
		json["dimmer"] = byte0;
		break;
	case IR_PROTOCOL_RUBICSONSTN:
		/*
			  aaaaaaaa 0dddsttt tttttttt hhhhhhhh wwwwwwww wwwwwwww
			0b11111001 11111001 00001100 10110010 11100011 01101111
		a = address = winddirection, s = sign, t = temperature, h = humidity, w = vindspeed
		*/
		windspeed_avg = (receiverBufferLong[4] & 0xFF)*0.8f;
		windspeed_max = (receiverBufferLong[5] & 0xFF)*0.8f;
		//var crc = data&0xFF;
		humidity = receiverBufferLong[3]&0xFF;
		temp = ((receiverBufferLong[1]&0x0F)<<8) + receiverBufferLong[2];
		winddirection = (((receiverBufferLong[1])>>4)&0x7)*360.0f/8;
		addr = receiverBufferLong[0];
		sign = (temp>>11)&1;
		if (sign > 0)
		{
		temp = temp^0xFFF;
		temp += 1;
		temp = -temp;
		}
		temp2 = temp/10.0f;
		/*
			aaaaaaaa 1------- -------b rrrrrrrr rrrrrrrr rrrrrrrr
		  0b11111001 11111001 00001100 10110010 11100011 01101111
		a = address b = battery empty, r = rain
		*/
		battery = receiverBufferLong[8] & 0x1;
		//var crc = data&0xFF;
		rain = 0.4f*((receiverBufferLong[9]*65536)+(receiverBufferLong[10]*256)+(receiverBufferLong[11]));
		json["avgWind"] = windspeed_avg;
		json["maxWind"] = windspeed_max;
		json["direction"] = winddirection;
		json["temperature"] = temp2;
		json["humidity"] = humidity;
		json["rain"] = rain;
		if (battery)
		{
			json["battery"] = "LOW";
		} else
		{
			json["battery"] = "OK";
		}
		json["address"] = addr;
		break;
	case IR_PROTOCOL_RUBICSON:
		/*
				  ??? aaaaaaaa sttt tttttttt hhhhhhhh cccccccc
		341731651126	0b100 11111001 000011001011 00101110 00110110	20.3 46%
		a = address, s = sign, t = temperature, h = humidity, c = crc
		*/
		crc = nReceivedValue&0xFF;
		humidity = (nReceivedValue>>8)&0xFF;
		temp = (nReceivedValue>>16)&0xFFF;
		addr = (nReceivedValue>>28)&0x3;
		//var calccrc=crc8(rshift(data,8), 32);
		sign = (temp>>11)&1;
		if (sign > 0)
		{
			temp = temp^0xFFF;
			temp += 1;
			temp = -temp;
		}
		temp2 = temp/10;

		json["temperature"] = temp2;
		json["humidity"] = humidity;
		json["crc"] = crc;
		json["address"] = addr;
		break;
	case IR_PROTOCOL_VIKINGSTEAK:
		addr = (nReceivedValue>>4)&0xFF;
		byte0 = (nReceivedValue>>32)&0xF;
		byte1 = (nReceivedValue>>28)&0xF;
		byte2 = (nReceivedValue>>24)&0xF;
		temp2 = roundf((float)(((((byte1^byte2)<<8)+((byte0^byte1)<<4)+byte0^10)-122)*5.0f)/9.0f);
		json["temperature"] = temp2;
		json["address"] = addr;
		break;
	default:
		return false;
		break;
	}
	json["protocol"] = this->nReceivedProtocol;
	return true;
}
/**
 *
 */
bool RCSwitch::receiveProtocol1(unsigned int changeCount){
    
      unsigned long code = 0;
      unsigned long delay = RCSwitch::timings[0] / 31;
      unsigned long delayTolerance = delay * RCSwitch::nReceiveTolerance * 0.01;    

      for (int i = 1; i<changeCount ; i=i+2) {
      
          if (RCSwitch::timings[i] > delay-delayTolerance && RCSwitch::timings[i] < delay+delayTolerance && RCSwitch::timings[i+1] > delay*3-delayTolerance && RCSwitch::timings[i+1] < delay*3+delayTolerance) {
            code = code << 1;
          } else if (RCSwitch::timings[i] > delay*3-delayTolerance && RCSwitch::timings[i] < delay*3+delayTolerance && RCSwitch::timings[i+1] > delay-delayTolerance && RCSwitch::timings[i+1] < delay+delayTolerance) {
            code+=1;
            code = code << 1;
          } else {
            // Failed
            i = changeCount;
            code = 0;
          }
      }      
      code = code >> 1;
    if (changeCount > 6) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
      RCSwitch::nReceivedValue = code;
      RCSwitch::nReceivedBitlength = changeCount / 2;
      RCSwitch::nReceivedDelay = delay;
      RCSwitch::nReceivedProtocol = 1;
    }

    if (code == 0){
        return false;
    }else if (code != 0){
        return true;
    }
    

}

bool RCSwitch::receiveProtocol2(unsigned int changeCount){
    
      unsigned long code = 0;
      unsigned long delay = RCSwitch::timings[0] / 10;
      unsigned long delayTolerance = delay * RCSwitch::nReceiveTolerance * 0.01;    

      for (int i = 1; i<changeCount ; i=i+2) {
      
          if (RCSwitch::timings[i] > delay-delayTolerance && RCSwitch::timings[i] < delay+delayTolerance && RCSwitch::timings[i+1] > delay*2-delayTolerance && RCSwitch::timings[i+1] < delay*2+delayTolerance) {
            code = code << 1;
          } else if (RCSwitch::timings[i] > delay*2-delayTolerance && RCSwitch::timings[i] < delay*2+delayTolerance && RCSwitch::timings[i+1] > delay-delayTolerance && RCSwitch::timings[i+1] < delay+delayTolerance) {
            code+=1;
            code = code << 1;
          } else {
            // Failed
            i = changeCount;
            code = 0;
          }
      }      
      code = code >> 1;
    if (changeCount > 6) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
      RCSwitch::nReceivedValue = code;
      RCSwitch::nReceivedBitlength = changeCount / 2;
      RCSwitch::nReceivedDelay = delay;
      RCSwitch::nReceivedProtocol = 2;
    }

    if (code == 0){
        return false;
    }else if (code != 0){
        return true;
    }

}

/** Protocol 3 is used by BL35P02.
 *
 */
bool RCSwitch::receiveProtocol3(unsigned int changeCount){
    
      unsigned long code = 0;
      unsigned long delay = RCSwitch::timings[0] / PROTOCOL3_SYNC_FACTOR;
      unsigned long delayTolerance = delay * RCSwitch::nReceiveTolerance * 0.01;    

      for (int i = 1; i<changeCount ; i=i+2) {
      
          if  (RCSwitch::timings[i]   > delay*PROTOCOL3_0_HIGH_CYCLES - delayTolerance
            && RCSwitch::timings[i]   < delay*PROTOCOL3_0_HIGH_CYCLES + delayTolerance
            && RCSwitch::timings[i+1] > delay*PROTOCOL3_0_LOW_CYCLES  - delayTolerance
            && RCSwitch::timings[i+1] < delay*PROTOCOL3_0_LOW_CYCLES  + delayTolerance) {
            code = code << 1;
          } else if (RCSwitch::timings[i]   > delay*PROTOCOL3_1_HIGH_CYCLES - delayTolerance
                  && RCSwitch::timings[i]   < delay*PROTOCOL3_1_HIGH_CYCLES + delayTolerance
                  && RCSwitch::timings[i+1] > delay*PROTOCOL3_1_LOW_CYCLES  - delayTolerance
                  && RCSwitch::timings[i+1] < delay*PROTOCOL3_1_LOW_CYCLES  + delayTolerance) {
            code+=1;
            code = code << 1;
          } else {
            // Failed
            i = changeCount;
            code = 0;
          }
      }      
      code = code >> 1;
      if (changeCount > 6) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
        RCSwitch::nReceivedValue = code;
        RCSwitch::nReceivedBitlength = changeCount / 2;
        RCSwitch::nReceivedDelay = delay;
        RCSwitch::nReceivedProtocol = 3;
      }

      if (code == 0){
        return false;
      }else if (code != 0){
        return true;
      }
}


/**
 * Test data on NEXA protocol
 * http://elektronikforumet.com/wiki/index.php?title=RF_Protokoll_-_Nexa_sj%C3%A4lvl%C3%A4rande
 * http://pastebin.com/PJX3bRAs
 */
bool RCSwitch::receiveProtocolNexa2(unsigned int changeCount){
	/* check if we have correct amount of data */ 
	if (!(changeCount == 131 || changeCount == 147)) {
		return false;
	}
	uint16 i;
	i=0;
	
	if ((RCSwitch::timings[i] < IR_NEXA2_START1 - IR_NEXA2_START1/IR_NEXA2_TOL_DIV) || (RCSwitch::timings[i] > IR_NEXA2_START1 + IR_NEXA2_START1/IR_NEXA2_TOL_DIV)) { //check start bit;
		return false;
	}
	i++;
	if ((RCSwitch::timings[i] < IR_NEXA2_HIGH - IR_NEXA2_HIGH/IR_NEXA2_TOL_DIV) || (RCSwitch::timings[i] > IR_NEXA2_HIGH + IR_NEXA2_HIGH/IR_NEXA2_TOL_DIV)) { //check start bit
		return false;
	}
	i++;
	if ((RCSwitch::timings[i] < IR_NEXA2_START2 - IR_NEXA2_START2/IR_NEXA2_TOL_DIV) || (RCSwitch::timings[i] > IR_NEXA2_START2 + IR_NEXA2_START2/IR_NEXA2_TOL_DIV)) { //check start bit
		return false;
	}
	i++;
	/* Incoming data could actually be longer than 32bits when a dimming command is received */
	uint64 rawbitsTemp = 0;
	uint64 bitCounterValue = 1;
	uint8 bitCounterAll = 0;
	uint8 bitCounter = 0;
	uint8 i2;
	uint8 endvalue = 127;
	if (changeCount == 147)
		endvalue = 143;
	for (i2 = 0; i2 < endvalue; i2++) {
		i++;
		if ((i2&1) == 0) {		/* if even, data */
			if ((bitCounterAll&1) == 0)
			{
				/* check length of transmit pulse */
				if ((RCSwitch::timings[i] > IR_NEXA2_LOW_ONE - IR_NEXA2_LOW_ONE/IR_NEXA2_TOL_DIV) && (RCSwitch::timings[i] < IR_NEXA2_LOW_ONE + IR_NEXA2_LOW_ONE/IR_NEXA2_TOL_DIV)) {
					/* write a one */
					rawbitsTemp += bitCounterValue;
				} else if ((RCSwitch::timings[i] > IR_NEXA2_LOW_ZERO - IR_NEXA2_LOW_ZERO/IR_NEXA2_TOL_DIV) && (RCSwitch::timings[i] < IR_NEXA2_LOW_ZERO + IR_NEXA2_LOW_ZERO/IR_NEXA2_TOL_DIV)) {
					/* do nothing, a zero is already in rawbits */
				} else {
					return false;
				}
				bitCounterValue = bitCounterValue + bitCounterValue;
				bitCounter++;
			}
			bitCounterAll++;
		} else {			/* if odd, no data */
			if ((RCSwitch::timings[i] < IR_NEXA2_HIGH - IR_NEXA2_HIGH/IR_NEXA2_TOL_DIV) || (RCSwitch::timings[i] > IR_NEXA2_HIGH + IR_NEXA2_HIGH/IR_NEXA2_TOL_DIV)) {
				return false;
			}
		}
	}
	RCSwitch::nReceivedValue = (unsigned long long)rawbitsTemp;
	RCSwitch::nReceivedBitlength = changeCount / 2;
	RCSwitch::nReceivedDelay = 100;
	RCSwitch::nReceivedProtocol = IR_PROTOCOL_NEXA2;
	return true;
}


bool RCSwitch::receiveProtocolRubicsonStation(unsigned int changeCount){
	/* check if we have correct amount of data */ 
	if (changeCount < 224) {
		return false;
	}
	uint8 bitIndex, byteIndex;

	/* Check start bit condition */
	uint16 i,i2;
	i=0;
	
	if (((RCSwitch::timings[i] < IR_RUBICSONSTN_LOW_START - IR_RUBICSONSTN_LOW_START/IR_RUBICSONSTN_TOL_DIV) || (RCSwitch::timings[i] > IR_RUBICSONSTN_LOW_START + IR_RUBICSONSTN_LOW_START/IR_RUBICSONSTN_TOL_DIV)) || ((RCSwitch::timings[i+1] < IR_RUBICSONSTN_HIGH - IR_RUBICSONSTN_HIGH/IR_RUBICSONSTN_TOL_DIV) || (RCSwitch::timings[i+1] > IR_RUBICSONSTN_HIGH + IR_RUBICSONSTN_HIGH/IR_RUBICSONSTN_TOL_DIV)))
	{
		i++;
		if (((RCSwitch::timings[i] < IR_RUBICSONSTN_LOW_START - IR_RUBICSONSTN_LOW_START/IR_RUBICSONSTN_TOL_DIV) || (RCSwitch::timings[i] > IR_RUBICSONSTN_LOW_START + IR_RUBICSONSTN_LOW_START/IR_RUBICSONSTN_TOL_DIV)) || ((RCSwitch::timings[i+1] < IR_RUBICSONSTN_HIGH - IR_RUBICSONSTN_HIGH/IR_RUBICSONSTN_TOL_DIV) || (RCSwitch::timings[i+1] > IR_RUBICSONSTN_HIGH + IR_RUBICSONSTN_HIGH/IR_RUBICSONSTN_TOL_DIV)))
		{
			i++;
			if (((RCSwitch::timings[i] < IR_RUBICSONSTN_LOW_START - IR_RUBICSONSTN_LOW_START/IR_RUBICSONSTN_TOL_DIV) || (RCSwitch::timings[i] > IR_RUBICSONSTN_LOW_START + IR_RUBICSONSTN_LOW_START/IR_RUBICSONSTN_TOL_DIV)) || ((RCSwitch::timings[i+1] < IR_RUBICSONSTN_HIGH - IR_RUBICSONSTN_HIGH/IR_RUBICSONSTN_TOL_DIV) || (RCSwitch::timings[i+1] > IR_RUBICSONSTN_HIGH + IR_RUBICSONSTN_HIGH/IR_RUBICSONSTN_TOL_DIV)))
			{
				return false;
			}
		}
	}
	i++;
	byteIndex = 0;
	bitIndex = 0;
	for (i2 = 0; i2 <= 223; i2++)
	{
		if ((i2&1) == 0)
		{		/* if odd, no data */
			if ((RCSwitch::timings[i] < IR_RUBICSONSTN_HIGH - IR_RUBICSONSTN_HIGH/IR_RUBICSONSTN_TOL_DIV) || (RCSwitch::timings[i] > IR_RUBICSONSTN_HIGH + IR_RUBICSONSTN_HIGH/IR_RUBICSONSTN_TOL_DIV))
			{
				return false;
			}
		} 
		else
		{	/* if even, data */
			/* check length of transmit pulse */
			if ((RCSwitch::timings[i] > IR_RUBICSONSTN_LOW_ONE - IR_RUBICSONSTN_LOW_ONE/IR_RUBICSONSTN_TOL_DIV) && (RCSwitch::timings[i] < IR_RUBICSONSTN_LOW_ONE + IR_RUBICSONSTN_LOW_ONE/IR_RUBICSONSTN_TOL_DIV))
			{
				/* write a one */
				RCSwitch::receiverBufferLong[byteIndex] = RCSwitch::receiverBufferLong[byteIndex]<<1;
				RCSwitch::receiverBufferLong[byteIndex] |= 1;
				bitIndex++;
			}
			else if ((RCSwitch::timings[i] > IR_RUBICSONSTN_LOW_ZERO - IR_RUBICSONSTN_LOW_ZERO/IR_RUBICSONSTN_TOL_DIV) && (RCSwitch::timings[i] < IR_RUBICSONSTN_LOW_ZERO + IR_RUBICSONSTN_LOW_ZERO/IR_RUBICSONSTN_TOL_DIV))
			{
				/* do nothing, a zero is already in rawbits */
				RCSwitch::receiverBufferLong[byteIndex] = RCSwitch::receiverBufferLong[byteIndex]<<1;
				bitIndex++;
			}
			else 
			{
				return false;
			}
			if (bitIndex == 8)
			{
			  byteIndex++;
			  bitIndex=0;
			}
		}
		i++;
	}
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
	RCSwitch::receiverBufferLong[0] = RCSwitch::receiverBufferLong[1] ^ RCSwitch::receiverBufferLong[2];
	RCSwitch::receiverBufferLong[1] = (RCSwitch::receiverBufferLong[4] & 0x0Fu) + ((RCSwitch::receiverBufferLong[4] & 0xE0u)>>1);
	RCSwitch::receiverBufferLong[2] = RCSwitch::receiverBufferLong[5];
	RCSwitch::receiverBufferLong[4] = RCSwitch::receiverBufferLong[7];
	RCSwitch::receiverBufferLong[5] = RCSwitch::receiverBufferLong[8];
	
	RCSwitch::receiverBufferLong[8] = RCSwitch::receiverBufferLong[3];
	RCSwitch::receiverBufferLong[3] = RCSwitch::receiverBufferLong[6];
	
	RCSwitch::receiverBufferLong[6] = RCSwitch::receiverBufferLong[0];
	RCSwitch::receiverBufferLong[7] = 0x80; //Second packet
	//RCSwitch::receiverBufferLong[9] = RCSwitch::receiverBufferLong[9];
	//RCSwitch::receiverBufferLong[10] = RCSwitch::receiverBufferLong[10];
	//RCSwitch::receiverBufferLong[11] = RCSwitch::receiverBufferLong[11];
	
	RCSwitch::nReceivedValue = (unsigned long long)(123456789);
	RCSwitch::nReceivedBitlength = changeCount / 2;
	RCSwitch::nReceivedDelay = 100;
	RCSwitch::nReceivedProtocol = IR_PROTOCOL_RUBICSONSTN;
	return true;
}


/**
 * Test data on Rubicson temperature sensor protocol
 *
 */

bool RCSwitch::receiveProtocolRubicsonTemperature(unsigned int changeCount){
	/* check if we have correct amount of data */
	if (changeCount != 73) {
		return false;
	}

	uint8 i, i2;
	uint64 rawbitsTemp = 0;

	i=0;
	if ((RCSwitch::timings[i] < IR_RUBICSON_LOW_START - IR_RUBICSON_LOW_START/IR_RUBICSON_TOL_DIV) || (RCSwitch::timings[i] > IR_RUBICSON_LOW_START + IR_RUBICSON_LOW_START/IR_RUBICSON_TOL_DIV))
	{
		return false;
	}

	for (i = 1; i < 74; i++)
	{
		if ((i&1) != 0)
		{		/* if odd, no data */
			if ((RCSwitch::timings[i] < IR_RUBICSON_HIGH - IR_RUBICSON_HIGH/IR_RUBICSON_TOL_DIV) || (RCSwitch::timings[i] > IR_RUBICSON_HIGH + IR_RUBICSON_HIGH/IR_RUBICSON_TOL_DIV))
			{
				return false;
			}
		}
		else
		{	/* if even, data */
			/* check length of transmit pulse */
			if ((RCSwitch::timings[i] > IR_RUBICSON_LOW_ONE - IR_RUBICSON_LOW_ONE/IR_RUBICSON_TOL_DIV) && (RCSwitch::timings[i] < IR_RUBICSON_LOW_ONE + IR_RUBICSON_LOW_ONE/IR_RUBICSON_TOL_DIV))
			{
				/* write a one */
				rawbitsTemp = rawbitsTemp<<1;
				rawbitsTemp |= 1;
			}
			else if ((RCSwitch::timings[i] > IR_RUBICSON_LOW_ZERO - IR_RUBICSON_LOW_ZERO/IR_RUBICSON_TOL_DIV) && (RCSwitch::timings[i] < IR_RUBICSON_LOW_ZERO + IR_RUBICSON_LOW_ZERO/IR_RUBICSON_TOL_DIV))
			{
				/* do nothing, a zero is already in rawbits */
				rawbitsTemp = rawbitsTemp<<1;
			}
			else
			{
				return false;
			}
		}
	}
	RCSwitch::nReceivedValue = (unsigned long long)rawbitsTemp;
	RCSwitch::nReceivedBitlength = changeCount / 2;
	RCSwitch::nReceivedDelay = 100;
	RCSwitch::nReceivedProtocol = IR_PROTOCOL_RUBICSON;
	return true;
}


/**
 * Test data on Viking steak temperature sensor protocol
 */
bool RCSwitch::receiveProtocolVikingSteak(unsigned int changeCount){
	/* check if we have correct amount of data */
	if (changeCount != 73) {
		return false;
	}
	uint8 i, i2;
	uint64 rawbitsTemp = 0;
	i=0;
	if ((RCSwitch::timings[i] < IR_VIKING_STEAK_LOW_START - IR_VIKING_STEAK_LOW_START/IR_VIKING_STEAK_TOL_DIV) || (RCSwitch::timings[i] > IR_VIKING_STEAK_LOW_START + IR_VIKING_STEAK_LOW_START/IR_VIKING_STEAK_TOL_DIV))
	{
		return false;
	}

	for (i = 1; i < 74; i++)
	{
		if ((i&1) != 0)
		{		/* if odd, no data */
			if ((RCSwitch::timings[i] < IR_VIKING_STEAK_HIGH - IR_VIKING_STEAK_HIGH/IR_VIKING_STEAK_TOL_DIV) || (RCSwitch::timings[i] > IR_VIKING_STEAK_HIGH + IR_VIKING_STEAK_HIGH/IR_VIKING_STEAK_TOL_DIV))
			{
				return false;
			}
		}
		else
		{	/* if even, data */
			/* check length of transmit pulse */
			if ((RCSwitch::timings[i] > IR_VIKING_STEAK_LOW_ONE - IR_VIKING_STEAK_LOW_ONE/IR_VIKING_STEAK_TOL_DIV) && (RCSwitch::timings[i] < IR_VIKING_STEAK_LOW_ONE + IR_VIKING_STEAK_LOW_ONE/IR_VIKING_STEAK_TOL_DIV))
			{
				/* write a one */
				rawbitsTemp = rawbitsTemp<<1;
				rawbitsTemp |= 1;
			}
			else if ((RCSwitch::timings[i] > IR_VIKING_STEAK_LOW_ZERO - IR_VIKING_STEAK_LOW_ZERO/IR_VIKING_STEAK_TOL_DIV) && (RCSwitch::timings[i] < IR_VIKING_STEAK_LOW_ZERO + IR_VIKING_STEAK_LOW_ZERO/IR_VIKING_STEAK_TOL_DIV))
			{
				/* do nothing, a zero is already in rawbits */
				rawbitsTemp = rawbitsTemp<<1;
			}
			else
			{
				return false;
			}
		}
	}
	RCSwitch::nReceivedValue = (unsigned long long)rawbitsTemp;
	RCSwitch::nReceivedBitlength = changeCount / 2;
	RCSwitch::nReceivedDelay = 100;
	RCSwitch::nReceivedProtocol = IR_PROTOCOL_VIKINGSTEAK;
	return true;
}

void RCSwitch::handleInterrupt() {

  static unsigned int duration;
  static unsigned int changeCount;
  static unsigned long lastTime;
  static unsigned int repeatCount;

  long time = micros();
  duration = time - lastTime;
  
  if (duration < 170) {
    changeCount = 0;
	repeatCount = 0;
  } else if (duration > 4500 && changeCount > 60) {
    repeatCount++;
    changeCount--;
    if (repeatCount == 1) {
	  RCSwitch::nReceivedValue = changeCount;
	  RCSwitch::nReceivedProtocol = 0;
	  if (!receiveProtocolNexa2(changeCount))
		if (!receiveProtocolRubicsonStation(changeCount))
		  if (!receiveProtocolRubicsonTemperature(changeCount))
		    if (!receiveProtocolVikingSteak(changeCount))
		      ;
      repeatCount = 0;
    }
    changeCount = 0;
  } else if (duration > 4500) {
    changeCount = 0;
  }

  if (changeCount >= RCSWITCH_MAX_CHANGES) {
    changeCount = 0;
    repeatCount = 0;
  }
  
  RCSwitch::timings[changeCount++] = duration;
  lastTime = time;  
}
#endif


