////////////////////////////////////////////
//      Temperature Sensor Device         //
//          Version 1.0                   //
//    (c) 2019 Osama Oransa               //
////////////////////////////////////////////
// Features:                              //
// 1. Supports 2 sensors: Temp/Humididity //
// 2. Supports 2 relay modules attached   //
// 3. Local alarm enabled                 //
// 4. Execution of smart rules if needed  //
// 5. Upgrade the firmware using OTA      //
// 6. Configurable alert values           //
// 7. Configurable loop & ping values     //
// 8. Seed Wifi configuration via Serial  //
////////////////////////////////////////////
// Connectivity:                          //
//  1. DHT sensor to 5v, GND and D6       //
//  2. Optional Buzzer: GND and D7        //
//  3. Optional Relay 1: GND & D5         //
//  4. Optional Relay 2: GND & D8 or D7   //
////////////////////////////////////////////
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266httpUpdate.h>
#include <base64.h>
#include <EEPROM.h>
#include "dht.h"
//Different IoT device variables
dht DHT;
#define sensorPin D6
#define sensorPin2 A0
#define RELAY_PIN D5
#define RELAY_PIN2 D8
#define BUZZER_PIN D7
//Connected Sensors Count 
int connectedSensors = 1;
boolean sensor2AsOutput=false;
//Connected Relay Modules Count 
int connectedDevices = 0;
//Is Buzzer attached for local alarm?
boolean alarmEnabled=true;
int currentVersion = 1;
int serverVersion = 0;
String deviceModel= "d1";//"d1_mini";
String SHOW_PASSWORD_KEY = "K12345"; //can be different per device model
//NOTE: first installation is version 1 and all variables must have actual values
//subsequent versions should only use placeholders not actual values and during OTA upgrade
//the actual values will be injected in the backend
/* Placeholders for the actual values for version > 1
String deviceId = "DEVICE_ID#DEVICE_ID#DEVICE_ID#DEVICE_ID";
String devicepassword = "DEVICE_PASSWORD#DEVICE_PASSWORD#DEVICE_PASSWORD";
String hostStr = "HOST_HOST_HOST_HOST_HOST";
String httpsStr = "PORT_HTTPS";
String httpStr = "HTTP_PORT";
String ssidStr = "WIFI_NAME_HERE#WIFI_NAME_HERE";
String passwordStr = "WIFI_PASS_HERE#WIFI_PASS_HERE";
*/
char* host ="hostAddresshostAddress";
String deviceId = "DEVICE_ID";
String devicepassword = "DEVICE_PASSWORD";
String hostStr = "HOST";
String httpsStr = "8181";
String httpStr = "8080";
int httpsPort = 0;
int httpPort = 0;
String ssidStr = "SSID";
String passwordStr = "WIFI_PASSWORD";
char* password = "wifiPasswordwifiPassword";
char* ssid = "SSIDSSIDSSIDSSIDSSIDSSID";
String sectetcode = "sectetcode";
int highAlert1=450;
int highAlert2=0;
int lowAlert1=0;
int lowAlert2=0;
String url = "/IoTHub/IoTServlet";
int action = 0;
const char* fingerprint = "41:B7:CA:88:4F:3C:EA:05:8A:43:59:7E:66:2E:BE:F8:6E:78:F6:1F";
//Remove this DEBUG definition line for production
//And keep it for development and testing ...
#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.print (x)
#define DEBUG_PRINTLN(x)  Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif
int loopCounter = 0;
int loopThreshold = 30; //number of loops before send update message each loop 6 seconds (5 delay + 1 logic)
int loopWaitTime = 6000;//wait time for each loop
int device_status = 0;  //relay module 1 status
int device_status2 = 0; //relay module 2 status
//smart rules are rules executed in the device as a results of high/low sensors readings
int smartRules1=0;
int smartRules2=0;
int previousStatus=1;
int previousMessageType=1;
boolean loginSuccess=false;
//the main loop method of the device
//here you implement the required logic and call the required helper methods from IoTDevice-Common file
void loop() {
  //check if serial is connected and has an input 
  //used to reset the wifi credentials
  if (Serial.available()) {
      String input = Serial.readString();
      input.trim();
      DEBUG_PRINTLN(input);
      updateVariables(input);
  }
  //1. Read the sensor data i.e. temperature and humidity ...
  DHT.read11(sensorPin);
  DEBUG_PRINT("Current humidity = ");
  DEBUG_PRINT(DHT.humidity);
  DEBUG_PRINT("%  ");
  DEBUG_PRINT("temperature = ");
  DEBUG_PRINT(DHT.temperature);
  DEBUG_PRINTLN("C  ");
  //2. construct payload message to IoT Server
  String message = String(DHT.temperature) + "," + String(DHT.humidity);
  int type = 1; //normal data message
  if (DHT.temperature == 0 && DHT.humidity == 0) {
    type = 6; // error message , must be faulty!
    loopCounter = loopThreshold; //to send message immediately
  } else if (DHT.temperature > highAlert1 || DHT.humidity > highAlert2) {
    type = 2; // high alert
    if(connectedDevices > 0 && (smartRules1!=0 || smartRules2!=0)){
      executeRulesHighAlert();  
    }    
    loopCounter = loopThreshold; //to send message immediately
  } else if (DHT.temperature < lowAlert1 || DHT.humidity < lowAlert2) {
    type = 4; // low alert
    if(connectedDevices > 0 && (smartRules1!=0 || smartRules2!=0)){
      executeRulesLowAlert();  
    }    
    loopCounter = loopThreshold; //to send message immediately
  }
  if (connectedDevices > 0){
    if (device_status == 0) message = message + ",F";
    else if (device_status == 1) message = message + ",N";
  }
  if (connectedDevices == 2) {
    if (device_status2 == 0) message = message + ",F";
    else if (device_status2 == 1) message = message + ",N";
  }
  //3. Add low battery signal to the message ..
  //This code is not working
  double curvolt = ESP.getVcc()/1024.00f;
  DEBUG_PRINTLN(curvolt);
  if(curvolt<2){ //1.8V is the operating minimal so check on 2.0
    message = message + ",L";
  }
  //4. connect the device to IoT Platform
  connectDevice(message, type);
  DEBUG_PRINTLN("========== End Loop code ===========");
  loopCounter++;
  delay(loopWaitTime);//Wait x seconds before accessing sensor again.
}
