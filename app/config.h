/*
 * config.h
 *
 *  Created on: 23 aug 2015
 *      Author: Linus
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#define PIN_RF_RX		12

//IR_MAX_PULSE_WIDTH=4000UL
//IR_MIN_STARTPULSE_WIDTH=10000UL

#define IR_MIN_PULSE_WIDTH		120
#define IR_MAX_PULSE_WIDTH		8000
//#define IR_MAX_PULSE_WIDTH		12000
#define MAX_NR_TIMES	512
// This may need to be changed if new protocols are introduced which have a lower number of pulses
#define MIN_NUM_PULSES	80


#define IR_PROTOCOLS_USE_SIRC	0
#define IR_PROTOCOLS_USE_RC5	0
#define IR_PROTOCOLS_USE_SHARP	0
#define IR_PROTOCOLS_USE_NEC	0
#define IR_PROTOCOLS_USE_SAMSUNG	0
#define IR_PROTOCOLS_USE_MARANTZ	0
#define IR_PROTOCOLS_USE_PANASONIC	0
#define IR_PROTOCOLS_USE_SKY	0
#define IR_PROTOCOLS_USE_NEXA2	1
#define IR_PROTOCOLS_USE_NEXA1	1
#define IR_PROTOCOLS_USE_VIKING	1
#define IR_PROTOCOLS_USE_VIKING_STEAK	1
#define IR_PROTOCOLS_USE_RUBICSON	1

#endif /* APP_CONFIG_H_ */
