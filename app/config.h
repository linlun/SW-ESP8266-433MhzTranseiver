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
#ifdef Old_PCB

#define PIN_DS18B20 5 // GPIO0
#define PIN_LED_WS2812 16 // GPIO2
#define PIN_RELAY 15 // GPIO2
#define PIN_OLED_SDA	12
#define PIN_OLED_SCL	13
#define PIN_ROTARY_A	4
#define PIN_ROTARY_B	14
#define PIN_ROTARY_SW	2
#else
#define PIN_DS18B20 5 // GPIO0
#define PIN_LED_WS2812 0 // GPIO2
#define PIN_RELAY 15 // GPIO2
#define PIN_OLED_SDA	12
#define PIN_OLED_SCL	13
#define PIN_ROTARY_A	14
#define PIN_ROTARY_B	4
#define PIN_ROTARY_SW	2
#define ROTARY_CHx_INVERT_DIRECTION	1
#endif

#define MAX_NUM_DS_SENSORS	6

#endif /* APP_CONFIG_H_ */
