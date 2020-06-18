// blink led by at commands via arduino serial
#include <SoftwareSerial.h>

const byte rxPin = 3; // Wire this to Tx Pin of ESP8266
const byte txPin = 2; // Wire this to Rx Pin of ESP8266

// We'll use a software serial interface to connect to ESP8266
SoftwareSerial ESP8266 (rxPin, txPin);

void setup() {
  Serial.begin(115200);
  ESP8266.begin(115200); // Change this to the baudrate used by ESP8266
  delay(1000); // Let the module self-initialize
  pinMode(12, OUTPUT);  // set the io pin 16 to output
}

void loop() {
  Serial.println("Sending an AT command...");
  ESP8266.println("AT+CIOWRITE=12,1");
  delay(30);
  while (ESP8266.available()){
     String inData = ESP8266.readStringUntil('\n');
     Serial.println("Got reponse from ESP8266: " + inData);
  } 
  delay(1000);
  ESP8266.println("AT+CIOWRITE=12,0");
  delay(30);
  while (ESP8266.available()){
     String inData = ESP8266.readStringUntil('\n');
     Serial.println("Got reponse from ESP8266: " + inData);
  }
  delay(1000); 
}
