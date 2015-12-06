## Local build configuration
## Parameters configured here will override default and ENV values.
## Uncomment and change examples:

#Add your source directories here separated by space
MODULES = app

## ESP_HOME sets the path where ESP tools and SDK are located.
## Windows:
# ESP_HOME = c:/Espressif

## MacOS / Linux:
#ESP_HOME = /opt/esp-open-sdk

## SMING_HOME sets the path where Sming framework is located.
## Windows:
#SMING_HOME = C:\Users\linus\Documents\GitHub\Sming\Sming
SMING_HOME = C:\Users\linus\Documents\GitHub\Sming-ArduinoJson_R5_Alpha\Sming


# MacOS / Linux
# SMING_HOME = /opt/sming/Sming

## COM port parameter is reqruied to flash firmware correctly.
## Windows: 
 COM_PORT = COM4

# MacOS / Linux:
# COM_PORT = /dev/tty.usbserial

# Com port speed
 COM_SPEED	= 460800
 #115200 230400
 COM_SPEED_SERIAL = 460800
 
 # SPIFFs Location
SPIFF_FILES = web/build