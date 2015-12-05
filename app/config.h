/*
 * config.h
 *
 *  Created on: 23 aug 2015
 *      Author: Linus
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#define WIFI_SSID  "Linuxz"
#define WIFI_PASS  "asdfghjkl"

#define PIN_RF_RX		55

//IR_MAX_PULSE_WIDTH=4000UL
//IR_MIN_STARTPULSE_WIDTH=10000UL

#define IR_MIN_PULSE_WIDTH		120
#define MAX_NR_TIMES	150
// This may need to be changed if new protocols are introduced which have a lower number of pulses
#define MIN_NUM_PULSES	60

#endif /* APP_CONFIG_H_ */
