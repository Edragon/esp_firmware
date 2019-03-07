# 1.2 version. (From 2017 Nov.)
* generate random number as topic, for example "ESP-123456"
* move server to iot2.electrodragon.com
* Blink LED <br>
0.5s on/off when config  
1s when connected to mqtt   
full off when not connect to mqtt  
* support format/reset, but will take 1-3 mintues, press btn2


# Notice
* If you flash any new firmware, please notice format spiffs, because it already format by our firmware.

# Used library

* #include <FS.h>                   //this needs to be first, or it all crashes and burns...
* #include <ESP8266WiFi.h>         //https://github.com/esp8266/Arduino

// needed for library 

* #include <DNSServer.h>
* #include <ESP8266WebServer.h>
* #include <WiFiManager_custom.h>          //https://github.com/tzapu/WiFiManager
* #include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
* #include <PubSubClient.h>
* #include <String.h>
* #include <Ticker.h>
