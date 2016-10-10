/*
 * AppSettings.h
 *
 *  Created on: 13 мая 2015 г.
 *      Author: Anakod
 */

#include "SmingCore.h"

#ifndef INCLUDE_APPSETTINGS_H_
#define INCLUDE_APPSETTINGS_H_

#define APP_SETTINGS_FILE ".settings.conf" // leading point for security reasons :)

// ... and/or MQTT username and password
#ifndef MQTT_USERNAME
	#define MQTT_USERNAME "linus"
	#define MQTT_PWD "linusPass"
#endif


// ... and/or MQTT host and port
#ifndef MQTT_HOST
	//#define MQTT_HOST "test.mosquitto.org"
	#define MQTT_HOST "hok.famlundin.org"
	#define MQTT_PORT 1883
#endif

struct ApplicationSettingsStorage
{
	String ssid = WIFI_SSID;
	String password = WIFI_PWD;

	bool dhcp = true;

	IPAddress ip;
	IPAddress netmask;
	IPAddress gateway;

	String mqtt_server = MQTT_HOST;
	String mqtt_user = MQTT_USERNAME;
	String mqtt_password = MQTT_PWD;
	uint32 mqtt_period = 1800;
	uint32 mqtt_port = MQTT_PORT;

	String mqtt_nodeName = "Lunch";

	//String ota_ROM_0 = "https://hok.famlundin.org:443/SW/SW-ESP8266_Lunch/rom0.bin";
	//String ota_SPIFFS = "https://hok.famlundin.org:443/SW/SW-ESP8266_Lunch/spiff_rom.bin";

	String ota_ROM_0 = "http://192.168.1.128:80/SW/SW-ESP8266_Lunch/rom0.bin";
	String ota_SPIFFS = "http://192.168.1.128:80/SW/SW-ESP8266_Lunch/spiff_rom.bin";

	void load()
	{
		DynamicJsonBuffer jsonBuffer;
		if (exist())
		{
			int size = fileGetSize(APP_SETTINGS_FILE);
			char* jsonString = new char[size + 1];
			fileGetContent(APP_SETTINGS_FILE, jsonString, size + 1);
			JsonObject& root = jsonBuffer.parseObject(jsonString);

			JsonObject& network = root["network"];
			ssid = network["ssid"].asString();
			password = network["password"].asString();

			dhcp = network["dhcp"];

			ip = network["ip"].asString();
			netmask = network["netmask"].asString();
			gateway = network["gateway"].asString();

			JsonObject& mqtt = root["mqtt"];
			mqtt_user = mqtt["user"].asString();
			mqtt_password = mqtt["password"].asString();
			mqtt_server = mqtt["server"].asString();
			mqtt_port = mqtt["port"];
			mqtt_period = mqtt["period"];
			mqtt_nodeName = mqtt["nodeName"].asString();

			JsonObject& ota = root["ota"];
			ota_ROM_0 = ota["rom0"].asString();
			ota_SPIFFS = ota["spiffs"].asString();

			delete[] jsonString;
		}
	}

	void save()
	{
		DynamicJsonBuffer jsonBuffer;
		JsonObject& root = jsonBuffer.createObject();

		JsonObject& network = jsonBuffer.createObject();
		root["network"] = network;
		network["ssid"] = ssid.c_str();
		network["password"] = password.c_str();

		network["dhcp"] = dhcp;

		// Make copy by value for temporary string objects
		network["ip"] = ip.toString();
		network["netmask"] = netmask.toString();
		network["gateway"] = gateway.toString();


		JsonObject& mqtt = jsonBuffer.createObject();
		root["mqtt"] = mqtt;
		mqtt["user"] = mqtt_user.c_str();
		mqtt["password"] = mqtt_password.c_str();
		mqtt["server"] = mqtt_server.c_str();
		mqtt["port"] = mqtt_port;
		mqtt["period"] = mqtt_period;

		mqtt["nodeName"] = mqtt_nodeName.c_str();

		JsonObject& ota = jsonBuffer.createObject();
		root["ota"] = ota;
		ota["rom0"] = ota_ROM_0.c_str();
		ota["spiffs"] = ota_SPIFFS.c_str();

		//TODO: add direct file stream writing
		String rootString;
		root.printTo(rootString);
		fileSetContent(APP_SETTINGS_FILE, rootString);
	}

	bool exist() { return fileExist(APP_SETTINGS_FILE); }
};

static ApplicationSettingsStorage AppSettings;

#endif /* INCLUDE_APPSETTINGS_H_ */
