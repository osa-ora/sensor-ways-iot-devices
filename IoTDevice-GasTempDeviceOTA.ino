////////////////////////////////////////////
//  GAS-Temperature Home Safety Device    //
//          Version 1.0                   //
//    (c) 2019 Osama Oransa               //
////////////////////////////////////////////
// Features:                              //
// 1. Supports 2 sensors: Temp & Gas      //
// 2. Supports 2 relay modules attached   //
// 3. Local alarm enabled                 //
// 4. Execution of smart rules if needed  //
// 5. Upgrade the firmware using OTA      //
// 6. Configurable alert values           //
// 7. Configurable loop & ping values     //
// 8. Seed device configuration via Serial//
////////////////////////////////////////////
// Connectivity:                          //
//  1. MQ-2 or any to 5v, GND and A0      //
//  2. DHT sensor to 5v, GND and D6       //
//  3. Optional Buzzer: GND and D7        //
//  4. Optional Relay 1: GND & D5         //
//  5. Optional Relay 2: GND & D8 or D7   //
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
#define sensorPin A0
#define sensorPin2 D6
#define RELAY_PIN D5
#define RELAY_PIN2 D8
#define BUZZER_PIN D7
//Connected Sensors Count 
int connectedSensors = 2;
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
//the actual values will be injected in the backend IoT Platform
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
String deviceId = "A16";//NOTE: Must be replace with the actual device Id or via Serial
String devicepassword = "123";//NOTE: Must be replace with the actual device password or via Serial
String hostStr = "0.0.0.0";//NOTE: Must be replaced with the actual backend server location
String httpsStr = "8181";//NOTE: Must be replace with the actual backend server port
String httpStr = "8080";//NOTE: Must be replace with the actual backend server port
int httpsPort = 0;
int httpPort = 0;
String ssidStr = "Kenzy";//NOTE: Must be replace with the actual wifi SSID or via Serial
String passwordStr = "Judy_2007";//NOTE: Must be replace with the actual wifi password or via Serial
char* password = "wifiPasswordwifiPassword";
char* ssid = "SSIDSSIDSSIDSSIDSSIDSSID";
String sectetcode = "sectetcode";
int highAlert1=450;
int highAlert2=0;
int lowAlert1=0;
int lowAlert2=0;
String url = "/IoTHub/IoTServlet";
int action = 0;
// Use web browser to view and copy
const char* fingerprint = "41:B7:CA:88:4F:3C:EA:05:8A:43:59:7E:66:2E:BE:F8:6E:78:F6:1F";
//NOTE: Remove this DEBUG definition line for production and keep it for development and testing environment...
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
//in case there is control devices connected i.e. relay modules
int smartRules1=0;
int smartRules2=0;
//flags
int previousStatus=1;
int previousMessageType=1;
boolean loginSuccess=false;
//the main loop method of the device
//here you implement the required logic and call the required helper methods from IoTDevice-Common file
void loop() {
  //check if serial is connected and has an input 
  //used to reset the wifi credentials or device id/password
  checkSerialInput();  
  //1. read the sensor data i.e. gas level ...
  double currentStatus=0;
  int loopSize=0;
  //loop for GAS reading ..
  while(loopSize<loopWaitTime){
    currentStatus=analogRead(sensorPin);
    //DEBUG_PRINTLN(currentStatus);
    currentStatus=(currentStatus/1024)*100;
    //DEBUG_PRINTLN(currentStatus);
    //if gas leakage and it is increasing send alert message
    //if decreasing wait for the upcoming update ..
    if(currentStatus>highAlert1 && currentStatus>previousStatus) break;
    delay(loopWaitTime/10);
    loopSize+=(loopWaitTime/10);
  }
  DHT.read11(sensorPin2);
  DEBUG_PRINT("currentGasStatus = ");
  DEBUG_PRINTLN(currentStatus);
  DEBUG_PRINT("Current humidity = ");
  DEBUG_PRINT(DHT.humidity);
  DEBUG_PRINT("%,");
  DEBUG_PRINT("temperature = ");
  DEBUG_PRINT(DHT.temperature);
  DEBUG_PRINTLN("C ");  
  int type = 1; //normal data message
  if (currentStatus>highAlert1) {
      type = 2; //high alert
      if(connectedDevices > 0 && (smartRules1!=0 || smartRules2!=0)){
        executeRulesHighAlert();  
      }      
      loopCounter = loopThreshold; //to send message immediately
  }
  //No Low Alert so no call to method:
  //executeRulesLowAlert();
  //if previous alert high then send the normal message i.e. no thing new
  //will be overriden by temp sensor 
  if(type==1 && (previousMessageType==2 || previousMessageType==4)) {
    loopCounter = loopThreshold;
  }
  if (DHT.temperature > highAlert2) {
      type = 2; // high alert
      if(connectedDevices > 0 && (smartRules1!=0 || smartRules2!=0)){
        executeRulesHighAlert();  
      }    
      loopCounter = loopThreshold; //to send message immediately
  }
  //2. construct payload message to IoT Server
  String message = String(currentStatus) + "," + String(DHT.temperature);
  if (connectedDevices > 0){
    if (device_status == 0) message = message + ",F";
    else if (device_status == 1) message = message + ",N";
  }
  if (connectedDevices == 2) {
    if (device_status2 == 0) message = message + ",F";
    else if (device_status2 == 1) message = message + ",N";
  }
  previousStatus=currentStatus;
  previousMessageType=type;
  //3. connect the device to IoT Platform
  connectDevice(message, type);
  //4. Play local alaram in case of high alert values
  if(type==2 && alarmEnabled) { //high alert
    playTone();
  }
  DEBUG_PRINTLN("========== End Loop code ===========");
  loopCounter++;
}
