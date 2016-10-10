#include <user_config.h>
#include "SmingCore.h"
#include <AppSettings.h>
#include <config.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
	#define WIFI_SSID "PleaseEnterSSID" // Put you SSID and Password here
	#define WIFI_PWD "PleaseEnterPass"
#endif

HttpServer server;
BssList networks;
String network, password;
String mqtt_client;
rBootHttpUpdate* otaUpdater = 0;

// Forward declarations
void startMqttClient();
void onMessageReceived(String topic, String message);
Timer procTimer, processTimer;
// MQTT client
// For quick check you can use: http://www.hivemq.com/demos/websocket-client/ (Connection= test.mosquitto.org:8080)
MqttClient *mqtt;

#include <RCSwitch.h> // library for controling Radio frequency switch

RCSwitch mySwitch = RCSwitch();

//Do we want to see trace for debugging purposes
#define TRACE 1  // 0= trace off 1 = trace on

void OtaUpdate_CallBack(bool result) {
	
	Serial.println("In callback...");
	if(result == true) {
		// success
		uint8 slot;
		slot = rboot_get_current_rom();
		if (slot == 0) slot = 1; else slot = 0;
		// set to boot new rom and then reboot
		Serial.printf("Firmware updated, rebooting to rom %d...\r\n", slot);
		rboot_set_current_rom(slot);
		System.restart();
	} else {
		// fail
		Serial.println("Firmware update failed!");
	}
}

void OtaUpdate() {
	
	uint8 slot;
	rboot_config bootconf;
	Serial.println("Updating...");
	procTimer.stop();
	mqtt->unsubscribe("+/Lunch");
	mqtt->unsubscribe("+/Firmware");
	
	// need a clean object, otherwise if run before and failed will not run again
	if (otaUpdater) delete otaUpdater;
	otaUpdater = new rBootHttpUpdate();
	
	// select rom slot to flash
	bootconf = rboot_get_config();
	slot = bootconf.current_rom;
	if (slot == 0) slot = 1; else slot = 0;

#ifndef RBOOT_TWO_ROMS
	// flash rom to position indicated in the rBoot config rom table
	otaUpdater->addItem(bootconf.roms[slot], AppSettings.ota_ROM_0);
#else
	// flash appropriate rom
	if (slot == 0) {
		otaUpdater->addItem(bootconf.roms[slot],AppSettings.ota_ROM_0);
	} else {
		otaUpdater->addItem(bootconf.roms[slot], AppSettings.ota_ROM_1);
	}
#endif
	
#ifndef DISABLE_SPIFFS
	// use user supplied values (defaults for 4mb flash in makefile)
	if (slot == 0) {
		otaUpdater->addItem(RBOOT_SPIFFS_0, AppSettings.ota_SPIFFS);
	} else {
		otaUpdater->addItem(RBOOT_SPIFFS_1, AppSettings.ota_SPIFFS);
	}
#endif

	// request switch and reboot on success
	//otaUpdater->switchToRom(slot);
	// and/or set a callback (called on failure or success without switching requested)
	otaUpdater->setCallback(OtaUpdate_CallBack);

	// start update
	otaUpdater->start();
}

void Switch() {
	uint8 before, after;
	before = rboot_get_current_rom();
	if (before == 0) after = 1; else after = 0;
	Serial.printf("Swapping from rom %d to rom %d.\r\n", before, after);
	rboot_set_current_rom(after);
	Serial.println("Restarting...\r\n");
	System.restart();
}

void ShowInfo() {
    Serial.printf("\r\nSDK: v%s\r\n", system_get_sdk_version());
    Serial.printf("Free Heap: %d\r\n", system_get_free_heap_size());
    Serial.printf("CPU Frequency: %d MHz\r\n", system_get_cpu_freq());
    Serial.printf("System Chip ID: %x\r\n", system_get_chip_id());
    Serial.printf("SPI Flash ID: %x\r\n", spi_flash_get_id());
    //Serial.printf("SPI Flash Size: %d\r\n", (1 << ((spi_flash_get_id() >> 16) & 0xff)));
}

void serialCallBack(Stream& stream, char arrivedChar, unsigned short availableCharsCount) {
	if (arrivedChar == '\n') {
		char str[availableCharsCount];
		for (int i = 0; i < availableCharsCount; i++) {
			str[i] = stream.read();
			if (str[i] == '\r' || str[i] == '\n') {
				str[i] = '\0';
			}
		}
		
		if (!strcmp(str, "connect")) {
			// connect to wifi
			WifiStation.config(WIFI_SSID, WIFI_PWD);
			WifiStation.enable(true);
		} else if (!strcmp(str, "ip")) {
			Serial.printf("ip: %s mac: %s\r\n", WifiStation.getIP().toString().c_str(), WifiStation.getMAC().c_str());
		} else if (!strcmp(str, "ota")) {
			OtaUpdate();
		} else if (!strcmp(str, "switch")) {
			Switch();
		} else if (!strcmp(str, "restart")) {
			System.restart();
		} else if (!strcmp(str, "ls")) {
			Vector<String> files = fileList();
			Serial.printf("filecount %d\r\n", files.count());
			for (unsigned int i = 0; i < files.count(); i++) {
				Serial.println(files[i]);
			}
		} else if (!strcmp(str, "cat")) {
			Vector<String> files = fileList();
			if (files.count() > 0) {
				Serial.printf("dumping file %s:\r\n", files[0].c_str());
				Serial.println(fileGetContent(files[0]));
			} else {
				Serial.println("Empty spiffs!");
			}
		} else if (!strcmp(str, "info")) {
			ShowInfo();
		} else if (!strcmp(str, "help")) {
			Serial.println();
			Serial.println("available commands:");
			Serial.println("  help - display this message");
			Serial.println("  ip - show current ip address");
			Serial.println("  connect - connect to wifi");
			Serial.println("  restart - restart the esp8266");
			Serial.println("  switch - switch to the other rom and reboot");
			Serial.println("  ota - perform ota update, switch rom and reboot");
			Serial.println("  info - show esp8266 info");
			Serial.println("  sl - Sleep for 5 seconds");
#ifndef DISABLE_SPIFFS
			Serial.println("  ls - list files in spiffs");
			Serial.println("  cat - show first file in spiffs");
#endif
			Serial.println();
		} else if (!strcmp(str, "sl")) {
			Serial.println("Going to sleep");
			delay(500);
			system_deep_sleep(5000000);
			delay(500);
			Serial.println("Wakeing up");
		} else {
			Serial.println("unknown command");
		}
	}
}

// Check for MQTT Disconnection
void checkMQTTDisconnect(TcpClient& client, bool flag){
	// Called whenever MQTT connection is failed.
	if (flag == true)
		Serial.println("MQTT Broker Disconnected!!");
	else
		Serial.println("MQTT Broker Unreachable!!");
	WifiAccessPoint.enable(true);
	// Restart connection attempt after few seconds
	procTimer.initializeMs(10 * 1000, startMqttClient).start(); // every 2 seconds
}



// Publish our message
void publishMessage()
{
	if (mqtt->getConnectionState() != eTCS_Connected)
		startMqttClient(); // Auto reconnect

	mqtt->publish(mqtt_client + "/Status", "Online"); // or publishWithQoS
}

// Callback for messages, arrived from MQTT server
void onMessageReceived(String topic, String message)
{
	if (topic.endsWith("Firmware"))
	{
		if (message.equals("UPDATE"))
		{
			OtaUpdate();
		}
	}
	if (topic.endsWith("Rf_Tx"))
	{
		if (topic.equals(mqtt_client + "/Rf_Tx"))
		{
			// My own message

		} else {
			;
		}
		if (message.equals("ON"))
		{
			
		}
		else
		{
		}
	}
	Serial.print(topic);
	Serial.print(":\r\n\t"); // Pretify alignment for printing
	Serial.println(message);
	WifiAccessPoint.enable(false);
}

// Run MQTT client

void startMqttClient()
{
	/*if (mqtt == null)
	{
		mqtt = new MqttClient(MQTT_HOST, MQTT_PORT, onMessageReceived);
	}
	*/
	procTimer.stop();
	mqtt_client = AppSettings.mqtt_nodeName + "_" + WifiStation.getMAC();
	if(!mqtt->setWill(mqtt_client + "/Status", "Offline", 1, true)) {
		debugf("Unable to set the last will and testament. Most probably there is not enough memory on the device.");
	}
	mqtt->connect(mqtt_client, MQTT_USERNAME, MQTT_PWD);
	// Assign a disconnect callback function
	mqtt->setCompleteDelegate(checkMQTTDisconnect);
	//mqtt->subscribe(mqtt_client + "/Rf_Tx");
	mqtt->subscribe("+/Rf_Tx");
	mqtt->subscribe("+/Firmware");
	mqtt->publish(mqtt_client + "/Status", "Online"); // or publishWithQoS
}
// Will be called when WiFi station was connected to AP
void connectOk()
{
	Serial.println("I'm CONNECTED");
//	ntpDemo = new ntpClientDemo();
	// Run MQTT client
	startMqttClient();
	WifiAccessPoint.enable(false);
	// Start publishing loop
	procTimer.initializeMs(20 * 1000, publishMessage).start(); // every 20 seconds
}

// Will be called when WiFi station timeout was reached
void connectFail()
{
	Serial.println("I'm NOT CONNECTED. Need help :(");
	WifiAccessPoint.enable(true);
	// .. some you code for device configuration ..
}

Timer connectionTimer;

void onIndex(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("index.html");
	auto &vars = tmpl->variables();
	response.sendTemplate(tmpl); // will be automatically deleted
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
	{
		AppSettings.dhcp = request.getPostParameter("dhcp") == "1";
		AppSettings.ip = request.getPostParameter("ip");
		AppSettings.netmask = request.getPostParameter("netmask");
		AppSettings.gateway = request.getPostParameter("gateway");
		debugf("Updating IP settings: %d", AppSettings.ip.isNull());
		AppSettings.save();
	}

	TemplateFileStream *tmpl = new TemplateFileStream("settings.html");
	auto &vars = tmpl->variables();

	bool dhcp = WifiStation.isEnabledDHCP();
	vars["dhcpon"] = dhcp ? "checked='checked'" : "";
	vars["dhcpoff"] = !dhcp ? "checked='checked'" : "";

	if (!WifiStation.getIP().isNull())
	{
		vars["ip"] = WifiStation.getIP().toString();
		vars["netmask"] = WifiStation.getNetworkMask().toString();
		vars["gateway"] = WifiStation.getNetworkGateway().toString();
	}
	else
	{
		vars["ip"] = "192.168.1.77";
		vars["netmask"] = "255.255.255.0";
		vars["gateway"] = "192.168.1.1";
	}

	response.sendTemplate(tmpl); // will be automatically deleted
}
void onMqttConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
	{
		AppSettings.mqtt_password = request.getPostParameter("password");
		AppSettings.mqtt_user = request.getPostParameter("user");
		AppSettings.mqtt_server = request.getPostParameter("adr");
		AppSettings.mqtt_period = request.getPostParameter("period").toInt();
		AppSettings.mqtt_port = request.getPostParameter("port").toInt();
		AppSettings.mqtt_nodeName = request.getPostParameter("nodeName");
		//debugf("Updating MQTT settings: %d", AppSettings.ip.isNull());
		AppSettings.save();
	}

	TemplateFileStream *tmpl = new TemplateFileStream("mqttsettings.html");
	auto &vars = tmpl->variables();

	vars["user"] = AppSettings.mqtt_user;
	vars["password"] = AppSettings.mqtt_password;
	vars["period"] = AppSettings.mqtt_period;
	vars["port"] = AppSettings.mqtt_port;
	vars["adr"] = AppSettings.mqtt_server;
	vars["nodeName"] = AppSettings.mqtt_nodeName;

	response.sendTemplate(tmpl); // will be automatically deleted
}

void onOtaConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
	{
		AppSettings.ota_ROM_0 = request.getPostParameter("rom0");
		AppSettings.ota_SPIFFS = request.getPostParameter("spiffs");
		AppSettings.save();
	}

	TemplateFileStream *tmpl = new TemplateFileStream("otasettings.html");
	auto &vars = tmpl->variables();

	vars["rom0"] = AppSettings.ota_ROM_0;
	vars["spiffs"] = AppSettings.ota_SPIFFS;

	response.sendTemplate(tmpl); // will be automatically deleted
}
void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

void onAjaxNetworkList(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	json["status"] = (bool)true;

	bool connected = WifiStation.isConnected();
	json["connected"] = connected;
	if (connected)
	{
		// Copy full string to JSON buffer memory
		json["network"]= WifiStation.getSSID();
	}

	JsonArray& netlist = json.createNestedArray("available");
	for (int i = 0; i < networks.count(); i++)
	{
		if (networks[i].hidden) continue;
		JsonObject &item = netlist.createNestedObject();
		item["id"] = (int)networks[i].getHashId();
		// Copy full string to JSON buffer memory
		item["title"] = networks[i].ssid;
		item["signal"] = networks[i].rssi;
		item["encryption"] = networks[i].getAuthorizationMethodName();
	}

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}
void onAjaxRunOta(HttpRequest &request, HttpResponse &response)
{
	OtaUpdate();
}
void makeConnection()
{
	WifiStation.enable(true);
	WifiStation.config(network, password);

	AppSettings.ssid = network;
	AppSettings.password = password;
	AppSettings.save();

	network = ""; // task completed
}

void onAjaxConnect(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	String curNet = request.getPostParameter("network");
	String curPass = request.getPostParameter("password");

	bool updating = curNet.length() > 0 && (WifiStation.getSSID() != curNet || WifiStation.getPassword() != curPass);
	bool connectingNow = WifiStation.getConnectionStatus() == eSCS_Connecting || network.length() > 0;

	if (updating && connectingNow)
	{
		debugf("wrong action: %s %s, (updating: %d, connectingNow: %d)", network.c_str(), password.c_str(), updating, connectingNow);
		json["status"] = (bool)false;
		json["connected"] = (bool)false;
	}
	else
	{
		json["status"] = (bool)true;
		if (updating)
		{
			network = curNet;
			password = curPass;
			debugf("CONNECT TO: %s %s", network.c_str(), password.c_str());
			json["connected"] = false;
			connectionTimer.initializeMs(1200, makeConnection).startOnce();
		}
		else
		{
			json["connected"] = WifiStation.isConnected();
			debugf("Network already selected. Current status: %s", WifiStation.getConnectionStatusName());
		}
	}

	if (!updating && !connectingNow && WifiStation.isConnectionFailed())
		json["error"] = WifiStation.getConnectionStatusName();

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void startWebServer()
{
	server.listen(80);
	server.addPath("/", onIndex);
	server.addPath("/ipconfig", onIpConfig);
	server.addPath("/mqttconfig", onMqttConfig);
	server.addPath("/ota", onOtaConfig);
	server.addPath("/ajax/get-networks", onAjaxNetworkList);
	server.addPath("/ajax/run-ota", onAjaxRunOta);
	server.addPath("/ajax/connect", onAjaxConnect);
	server.setDefaultHandler(onFile);
}


void networkScanCompleted(bool succeeded, BssList list)
{
	if (succeeded)
	{
		for (int i = 0; i < list.count(); i++)
			if (!list[i].hidden && list[i].ssid.length() > 0)
				networks.add(list[i]);
	}
	networks.sort([](const BssInfo& a, const BssInfo& b){ return b.rssi - a.rssi; } );
}

//trace
void trc(String msg){
  if (TRACE) {
  //Serial.println(msg);
  }
}

void trcu64(uint64 number)
{
  int digit;

  while(number) {
    digit = number % 10;
    number /= 10;
    Serial.print(digit);
    // Do something with digit
  }
}

void trc2(String msg){
  if (TRACE) {
  Serial.print(msg);
  }
}
void process()
{
	// Receive loop, if data received by RF433 send it by MQTT to MQTTsubject
	  if (mySwitch.available()) {
	    // Topic on which we will send data
	    trc("Receiving 433Mhz signal");
	    String MQTTsubject = "home/433toMQTT_rx";
	    unsigned long MQTTvalue;
	    MQTTvalue=mySwitch.getReceivedValue();
	    long debug = mySwitch.getReceivedDelay();
	    long debug2 = mySwitch.getReceivedBitlength();
	    long debug3 = mySwitch.getReceivedProtocol();
	    unsigned int* times = mySwitch.getReceivedRawdata();
	    uint8* times2 = mySwitch.getReceivedLongdata();
	    trc2("RAW: "+String(MQTTvalue)+" : "+String(debug)+" : "+String(debug2)+" : "+String(debug3)+"\r\n");
	    if (debug3 ==5)
	    {
	      /*
	      uint64 data = (uint64)times2[0] + ((uint64)times2[1]<<8) + ((uint64)times2[2]<<16) + ((uint64)times2[3]<<24) + ((uint64)times2[4]<<32) + ((uint64)times2[5]<<40);
	      trc2("data2:\r\n");
	      trcu64(data);
	             data = (uint64)times2[6] + ((uint64)times2[7]<<8) + ((uint64)times2[8]<<16) + ((uint64)times2[9]<<24) + ((uint64)times2[10]<<32) + ((uint64)times2[11]<<40);
	      trc2("data3:\r\n");
	      trcu64(data);
	      */

	      for (int j = 0 ; j < 12 ; j++)
	      {
	        Serial.print(times2[j], HEX);
	        Serial.print(" ");
	      }
	      Serial.println("");
	/*
	              aaaaaaaa 0dddsttt tttttttt hhhhhhhh wwwwwwww wwwwwwww
	            0b11111001 11111001 00001100 10110010 11100011 01101111
	      a = address = winddirection, s = sign, t = temperature, h = humidity, w = vindspeed
	      */
	      float windspeed_avg = (times2[4] & 0xFF)*0.8f;
	      float windspeed_max = (times2[5] & 0xFF)*0.8f;
	      //var crc = data&0xFF;
	      uint8 humidity = times2[3]&0xFF;
	      sint16 temp = ((times2[1]&0x0F)<<8) + times2[2];
	      float winddirection = (((times2[1])>>4)&0x7)*360.0f/8;
	      uint8 addr = times2[0];
	      uint8 sign = (temp>>11)&1;
	      if (sign > 0)
	      {
	        temp = temp^0xFFF;
	        temp += 1;
	        temp = -temp;
	      }
	      float temp2 = temp/10.0f;
	      /*
	            aaaaaaaa 1------- -------b rrrrrrrr rrrrrrrr rrrrrrrr
	          0b11111001 11111001 00001100 10110010 11100011 01101111
	      a = address b = battery empty, r = rain
	      */
	      uint8 battery = times2[8] & 0x1;
	      //var crc = data&0xFF;
	      float rain = 0.4f*((times2[9]*65536)+(times2[10]*256)+(times2[11]));

	      Serial.print("Wind avg: ");
	      Serial.print(windspeed_avg);
	      Serial.println("m/s");
	            Serial.print("Wind max: ");
	      Serial.print(windspeed_max);
	      Serial.println("m/s");
	            Serial.print("Temp:     ");
	      Serial.print(temp2);
	      Serial.println("c");
	            Serial.print("Humidity: ");
	      Serial.print(humidity);
	      Serial.println("%");
	            Serial.print("Wind dir: ");
	      Serial.print(winddirection);
	      Serial.println("deg");
	            Serial.print("Rain:     ");
	      Serial.print(rain);
	      Serial.println("mm");
	            Serial.print("Battery:  ");
	      Serial.print(battery);
	      Serial.println("");
	    }

	    if (MQTTvalue < 250 && MQTTvalue > 220)
	    {
	      for (int i = 0; i <=  MQTTvalue+2; i++)
	      {
	        trc2(String(times[i])+" ");
	      }
	      trc2("\r\n");
	    }

	    mySwitch.resetAvailable();
	    /*
	    if (client.connected()) {
	      trc("Sending 433Mhz signal to MQTT");
	      trc(String(MQTTvalue));
	      //sendMQTT(MQTTsubject,String(MQTTvalue));
	    } else {
	      if (reconnect()) {
	        sendMQTT(MQTTsubject,String(MQTTvalue));
	        lastReconnectAttempt = 0;
	      }
	    }
	    */
	  }

}


void init() {


	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Debug output to serial

	
	// mount spiffs
	int slot = rboot_get_current_rom();
#ifndef DISABLE_SPIFFS
	if (slot == 0) {
#ifdef RBOOT_SPIFFS_0
		debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_0 , SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_0, SPIFF_SIZE);
#else
		debugf("trying to mount spiffs at %x, length %d", 0x100000, SPIFF_SIZE);
		spiffs_mount_manual(0x100000, SPIFF_SIZE);
#endif
	} else {
#ifdef RBOOT_SPIFFS_1
		debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_1 , SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_1, SPIFF_SIZE);
#else
		debugf("trying to mount spiffs at %x, length %d", SPIFF_SIZE);
		spiffs_mount_manual(0x300000, SPIFF_SIZE);
#endif
	}
#else
	debugf("spiffs disabled");
#endif
	AppSettings.load();

	WifiAccessPoint.enable(false);
	// connect to wifi
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.enable(true);
	
	if (AppSettings.exist())
	{
		WifiStation.config(AppSettings.ssid, AppSettings.password);
		if (!AppSettings.dhcp && !AppSettings.ip.isNull())
			WifiStation.setIP(AppSettings.ip, AppSettings.netmask, AppSettings.gateway);
	}
	mqtt = new MqttClient(AppSettings.mqtt_server,AppSettings.mqtt_port, onMessageReceived);

	WifiStation.startScan(networkScanCompleted);

	// Start AP for configuration
	WifiAccessPoint.enable(true);
	WifiAccessPoint.config("Sming Configuration "+ WifiStation.getMAC(), "", AUTH_OPEN);

	// Run WEB server
	startWebServer();
	
	Serial.printf("\r\nCurrently running rom %d.\r\n", slot);
	Serial.println("Type 'help' and press enter for instructions.");
	Serial.println();
	
	Serial.setCallback(serialCallBack);
	// Run our method when station was connected to AP (or not connected)
	WifiStation.waitConnection(connectOk, 20, connectFail); // We recommend 20+ seconds for connection timeout at start

	mySwitch.enableTransmit(4); // RF Transmitter is connected to Arduino Pin #10
	  mySwitch.setRepeatTransmit(20); //increase transmit repeat to avoid lost of rf sendings
	  mySwitch.enableReceive(5);  // Receiver on inerrupt 0 => that is pin #2
	  processTimer.initializeMs(10, process).start(); // every 20 seconds

}
