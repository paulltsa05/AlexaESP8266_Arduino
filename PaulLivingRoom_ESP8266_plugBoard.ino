/*
 Version 0.1 - Feb 10 2018
*/ 

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h> //  https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries 
#include <ArduinoJson.h> // https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

#define MyApiKey "xxxxx" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard
#define MySSID "xxxxx" // TODO: Change to your Wifi network SSID
#define MyWifiPassword "xxxxx" // TODO: Change to your Wifi network password

#define API_ENDPOINT "http://sinric.com"
#define HEARTBEAT_INTERVAL 300000 // 5 Minutes

#define BOARD_LED     2
#define GPIO_OUT_D0   16	//LIGHT     - "Alexa turn On the Light" or "Alexa turn On the Light"
#define GPIO_OUT_D1   5		//Reserve	
#define GPIO_OUT_D2   4 	//FAN		- "Alexa turn On the Fan" or "Alexa turn On the Fan"

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

void turnOn(String deviceId) {
  if (deviceId == "5f25a55ead7a48327f3861b4") // Device ID of first device - LIGHT
  {  
    Serial.print("LIGHT Turn on device id: ");
    Serial.println(deviceId);
    digitalWrite(BOARD_LED, LOW);
    digitalWrite(GPIO_OUT_D0, HIGH);    
  } 
  else if (deviceId == "5f26a945ad7a48327f387fed") // Device ID of second device - FAN
  { 
    Serial.print("FAN Turn on device id: ");
    Serial.println(deviceId);
    digitalWrite(GPIO_OUT_D2, HIGH);
  }
  else if (deviceId == "5f26a95dad7a48327f387ff3") // Device ID of second device - Switch
  { 
    Serial.print("Switch Turn on device id: ");
    Serial.println(deviceId);
    digitalWrite(GPIO_OUT_D1, HIGH);
  }
  else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);    
  }     
}

void turnOff(String deviceId) {

    if (deviceId == "5f25a55ead7a48327f3861b4") // Device ID of first device - LIGHT
  { 
    Serial.print(LED_BUILTIN); 
    Serial.print("LIGHT Turn off device id: ");
    Serial.println(deviceId);
    digitalWrite(BOARD_LED, HIGH);
    digitalWrite(GPIO_OUT_D0, LOW);    
  } 
  else if (deviceId == "5f26a945ad7a48327f387fed") // Device ID of second device - FAN
  { 
    Serial.print("FAN Turn off device id: ");
    Serial.println(deviceId);
    digitalWrite(GPIO_OUT_D2, LOW);
  }
  else if (deviceId == "5f26a95dad7a48327f387ff3") // Device ID of second device - Switch
  { 
    Serial.print("Switch Turn off device id: ");
    Serial.println(deviceId);
    digitalWrite(GPIO_OUT_D1, LOW);
  }
  else {
    Serial.print("Turn off for unknown device id: ");
    Serial.println(deviceId);    
  }   
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
        // Example payloads

        // For Light device type
        // {"deviceId": xxxx, "action": "setPowerState", value: "ON"} // https://developer.amazon.com/docs/device-apis/alexa-powercontroller.html
        // {"deviceId": xxxx, "action": "AdjustBrightness", value: 3} // https://developer.amazon.com/docs/device-apis/alexa-brightnesscontroller.html
        // {"deviceId": xxxx, "action": "setBrightness", value: 42} // https://developer.amazon.com/docs/device-apis/alexa-brightnesscontroller.html
        // {"deviceId": xxxx, "action": "SetColor", value: {"hue": 350.5,  "saturation": 0.7138, "brightness": 0.6501}} // https://developer.amazon.com/docs/device-apis/alexa-colorcontroller.html
        // {"deviceId": xxxx, "action": "DecreaseColorTemperature"} // https://developer.amazon.com/docs/device-apis/alexa-colortemperaturecontroller.html
        // {"deviceId": xxxx, "action": "IncreaseColorTemperature"} // https://developer.amazon.com/docs/device-apis/alexa-colortemperaturecontroller.html
        // {"deviceId": xxxx, "action": "SetColorTemperature", value: 2200} // https://developer.amazon.com/docs/device-apis/alexa-colortemperaturecontroller.html
        
#if ARDUINOJSON_VERSION_MAJOR == 5
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
        DynamicJsonDocument json(1024);
        deserializeJson(json, (char*) payload);      
#endif        
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "setPowerState") { // Switch or Light
            String value = json ["value"];
            if(value == "ON") {
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }
        }
        else if(action == "SetColor") {
            // Alexa, set the device name to red
            // get text: {"deviceId":"xxxx","action":"SetColor","value":{"hue":0,"saturation":1,"brightness":1}}
            String hue = json ["value"]["hue"];
            String saturation = json ["value"]["saturation"];
            String brightness = json ["value"]["brightness"];

            Serial.println("[WSc] hue: " + hue);
            Serial.println("[WSc] saturation: " + saturation);
            Serial.println("[WSc] brightness: " + brightness);
        }
        else if(action == "SetBrightness") {
          
        }
        else if(action == "AdjustBrightness") {
          
        }
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BOARD_LED, OUTPUT);
  pinMode(GPIO_OUT_D0, OUTPUT);
  pinMode(GPIO_OUT_D1, OUTPUT);
  pinMode(GPIO_OUT_D2, OUTPUT);
  digitalWrite(BOARD_LED, HIGH);//High is OFF, LOW is ON
  digitalWrite(GPIO_OUT_D0, HIGH);
  digitalWrite(GPIO_OUT_D1, HIGH);
  digitalWrite(GPIO_OUT_D2, HIGH);
  
  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets
}

void loop() {
  webSocket.loop();
  
  if(isConnected) {
      uint64_t now = millis();
      
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");          
      }

  }   
}
