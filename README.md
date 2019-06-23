

## Hardware

### Required components
* Microcontroller of your choice:
    * ESP8266 ESP-12E ([Amazon](http://www.amazon.de/dp/B06Y1LZLLY/))
    * ESP8266 ESP-12E D1 Mini ([Amazon](http://www.amazon.de/dp/B0754N794H))
* Moisture sensor ([Amazon](http://www.amazon.de/dp/B075K94S5D/))

### Schematics
<img src="Schematics/moisture_sensor.png" height="500px">

## Software

IDE: [Arduino](https://www.arduino.cc/en/Main/Software)

### Dependencies
* ESP8266 Board ([Github](https://github.com/esp8266/Arduino))
* PubSubClient 2.7.0 ([Github](https://github.com/knolleary/pubsubclient))
* ArduinoJson 5.13.5 ([Github](https://github.com/bblanchon/ArduinoJson))
* WifiManager 0.14.0 ([Github](https://github.com/tzapu/WiFiManager))
* NTPClient 3.1.0 ([Github](https://github.com/arduino-libraries/NTPClient))

## Installation

1. Install Arduino IDE from link above
1. Add ESP8266 Addon (See tutorial from [Sparkfun](https://learn.sparkfun.com/tutorials/esp8266-thing-hookup-guide/installing-the-esp8266-arduino-addon))
1. Install all dependencies with library manager
   * *Sketch > Include Library > Manage Libraries*
   * Search for each dependency (see list above) except ESP8266 Board and install the appropriate versions.
1. Download this repository and open it with the IDE
1. Connect ESP8266 to the computer
1. Select the board from *Tools > Board > NodeMCU 1.0 (ESP-12E Module)* (This might vary depending on the board you are using)
1. Select the COM port where ESP8266 is connected *Tools > Board > Port > COMxx*
1. Push `RST` and `FLASH` button, release `RST` and then release `FLASH` (This might vary depending on the board you are using)
1. Flash the firmware from the Arduino IDE
1. The microcontroller will create a new access point. Connect there and navigate with a browser to `192.168.4.1`.
1. Provide in the webinterface the SSID and password for your network.
1. After the microcontroller connected to you network figure out the IP address of it. This can be done by:
    * checking serial monitor *Tools > Serial Monitor*
    * checking webinterface of your router
    * use a network scanning tool
1. Configure mqtt by issuing the following command: <br>`curl -i -X PUT -d {\"host\":\"host\",\"port\":1234,\"client\":\"clientname\",\"user\":\"user\",\"pwd\":\"password\",\"ssl\":false} http://<ip of µC>/api/config/mqtt` 
1. Configure actors by issuing the following command: <br>`curl -i -X PUT -d [{\"pin\":1,\"topic\":\"topic\"}] http://<ip of µC>/api/config/actors`
1. Configure sensors by issuing the following command: <br>`curl -i -X PUT -d "[{\"pin\":0,\"sensorName\":\"sensor name\",\"location\":\"location\",\"type\":\"water\"}]" http://<ip of µC>/api/config/sensors`
1. Restart the microcontroller
