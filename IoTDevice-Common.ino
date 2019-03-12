//this is the main setup method
//it has simple initilization for the pins, variables and login the device.
void setup(){
  //1. init EEPROM memory simulation
  EEPROM.begin(512);
  Serial.begin(115200);
  //to reset the device
  //saveToMemory(0,"",0);
  String test=readFromMemory(0);
  if(test!="IOT"){//if not clear it
    boolean notConfigured=true;
    clearValues();
    //wait for the initial configurations
    Serial.println("Waiting For The Initial Configurations via Serial:");
    while(notConfigured){
      checkSerialInput();
      delay(500);
    }
  }else{
    showValues(true);
  }
  //2. init pins
  initPins();  
  //3. initialize connection and other variables
  initializeVariables();
  //4. login wifi and to IoT server
  loginDevice();
  //send update message after login immediately by setting the loopCounter to loopThreshold
  loopCounter = loopThreshold;
}
//initialization of the required pins
void initPins(){
  //sledge-hammer approach :) :)
  if(alarmEnabled){
    pinMode(BUZZER_PIN,OUTPUT);
  }
  if(connectedSensors>0){
    pinMode(sensorPin,INPUT);
    /*analogWrite(sensorPin, 1024);
    analogWrite(sensorPin, 0);
    analogWrite(sensorPin, 1024);*/
  }
  if(connectedSensors>1){
    /*analogWrite(sensorPin2, 1024);
    analogWrite(sensorPin2, 0);
    analogWrite(sensorPin2, 1024);*/
    pinMode(sensorPin2,INPUT);    
  }
  if (connectedDevices > 0){
    pinMode(RELAY_PIN,OUTPUT);
    /*analogWrite(RELAY_PIN, 1024);
    analogWrite(RELAY_PIN, 0);
    analogWrite(RELAY_PIN, 1024);
    analogWrite(RELAY_PIN, 0);*/
    device_status=analogRead(RELAY_PIN);
  }
  if (connectedDevices == 2) {
    pinMode(RELAY_PIN2,OUTPUT);    
    /*analogWrite(RELAY_PIN2, 1024);
    analogWrite(RELAY_PIN2, 0);
    analogWrite(RELAY_PIN2, 1024);
    analogWrite(RELAY_PIN2, 0);*/
    device_status2=analogRead(RELAY_PIN2);
  }
}
//device login method
void loginDevice(){
  loginSuccess = false;
    while (!loginSuccess) {
      if (WiFi.status() != WL_CONNECTED) {
        connectToWifi();
      }
      Serial.println("Try to login the device...");
      sectetcode = login();
      //give a chance to set new configuration value, if login failed
      if (!loginSuccess){
        Serial.println("Login is not successful, You can update the configurations if invalid!");
        checkSerialInput();
        delay(6000);
      }
    }
    Serial.println("Device connected successfully to WiFi & IoT Platform");
    DEBUG_PRINTLN(sectetcode);
    DEBUG_PRINTLN("==========");
}
//this method send payload to the backend and receive the response
//it also process the required recieved actions from the backend 
//if wifi is disconnected, it connect it again.
//if device login required, it relogin the device again
void connectDevice(String param, int type){
  //send the payload message or ping to IoT Server
  //we will only send update message every x intervals of if an action required from the device or if alert message type
  //so if an action will be performed we will set the counter at loopThreshold
  if (loopCounter >= loopThreshold) {
    action = sendUpdateMessage(param, type);
    loopCounter = 0;
  } else {
    action = sendPingMessage(param, type);
  }
  //check the required action on this device
  //1 switch device on, 2 switch device off, 4 restart , 5 keep update the IoT server with your messages, 0 login required
  if (action == 1 && device_status == 0) {
    device_status = 1;
    //switch it on
    digitalWrite(RELAY_PIN, LOW); // Turns ON Relay 1
    DEBUG_PRINTLN("Turn ON Device 1");
    DEBUG_PRINTLN(digitalRead(RELAY_PIN));
    //TODO: to be fixed and removed or added to device 2 code!
    //for unknown reason turn on device, make led go low !
    //digitalWrite(ledPin, HIGH);    
    loopCounter = loopThreshold; //send message next loop
  } else if (action == 2 && device_status == 1) {
    device_status = 0;
    digitalWrite(RELAY_PIN, HIGH); // Turns OFF Relay 1
    DEBUG_PRINTLN("Turn OFF Device 1");
    DEBUG_PRINTLN(digitalRead(RELAY_PIN));
    loopCounter = loopThreshold; //send message next loop
    //switch it off
  } else if (action == 6 && device_status2 == 0) {
    device_status2 = 1;
    //switch it on
    digitalWrite(RELAY_PIN2, LOW); // Turns ON Relay 2
    DEBUG_PRINTLN("Turn ON Device 2");
    loopCounter = loopThreshold; //send message next loop
  } else if (action == 7 && device_status2 == 1) {
    device_status2 = 0;
    digitalWrite(RELAY_PIN2, HIGH); // Turns OFF Relay 2
    DEBUG_PRINTLN("Turn OFF Device 2");
    loopCounter = loopThreshold; //send message next loop
    //switch it off
  } else if (action == 9) {
    DEBUG_PRINTLN("Device Update Message request!");
    loopCounter = loopThreshold;
  } else if (action == 4) {
    DEBUG_PRINTLN("Device Restart request!");
    ESP.wdtDisable();
    ESP.restart();
    //need to reconnect to wifi, login again...
  } else if (action == 0) {
    loginDevice(); 
  }
  //ensure wifi is connected always
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }

}
//initialize the required variables
//needed to support dynamic injection of variable values during OTA upgrade
void initializeVariables(){
  String temp=readFromMemory(110);
  if(temp!=""){
    deviceId=temp;
  }
  deviceId.trim();
  String temp1=readFromMemory(160);
  if(temp1!=""){
    devicepassword=temp1;
  }
  devicepassword.trim();
  String temp2=readFromMemory(10);
  if(temp2!=""){
    ssidStr=temp2;
  }
  ssidStr.trim();
  String temp3=readFromMemory(60);
  if(temp3!=""){
    passwordStr=temp3;
  }
  passwordStr.trim();
  hostStr.trim();
  httpsStr.trim();
  httpStr.trim();
  DEBUG_PRINTLN(deviceId);DEBUG_PRINTLN(devicepassword);DEBUG_PRINTLN(hostStr);
  DEBUG_PRINTLN(ssidStr);DEBUG_PRINTLN(passwordStr);DEBUG_PRINTLN(httpsStr);DEBUG_PRINTLN(httpStr);
  int len=hostStr.length()+1;
  hostStr.toCharArray(host,len);
  DEBUG_PRINTLN(host);DEBUG_PRINTLN(ssid);DEBUG_PRINTLN(password);
  len=passwordStr.length()+1;
  passwordStr.toCharArray(password,len);
  DEBUG_PRINTLN(host);DEBUG_PRINTLN(ssid);DEBUG_PRINTLN(password);
  len=ssidStr.length()+1;
  ssidStr.toCharArray(ssid,len);
  DEBUG_PRINTLN(host);DEBUG_PRINTLN(ssid);DEBUG_PRINTLN(password);
  httpsPort=httpsStr.toInt();
}
//Wifi connecting method
void connectToWifi() {
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    //give a chance to set new configuration value, if login failed
    if(WiFi.status() != WL_CONNECTED){
      checkSerialInput();
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println(".");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
//Actual login methood
//this contains the actual call to the backend service
//it initializes the different variables values as per the backend values
//it checks if firmware upgrade is required using OTA
//it flags login success or not
String login() {
  DEBUG_PRINTLN("========== Login ===========");
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  DEBUG_PRINT("connecting using https port ");
  DEBUG_PRINTLN(httpsPort);
  DEBUG_PRINTLN(host);
  if (!client.connect(host, httpsPort)) {
    DEBUG_PRINTLN("connection failed");
    //digitalWrite(ledPin, LOW);
    action = 0; //relogin plz ...
    return sectetcode;    
  }
  if (client.verify(fingerprint, host)) {
    DEBUG_PRINTLN("certificate matches");
  } else {
    DEBUG_PRINTLN("certificate doesn't match");
    //digitalWrite(ledPin, LOW);
    //action = 0; //relogin plz ...
    //return sectetcode;    
  }
  //POST
  String connection = "t=0&a=0&i=" + base64::encode(deviceId) + "&p=" + base64::encode(devicepassword)+"&v="+currentVersion;
  DEBUG_PRINTLN(connection);
  int n = connection.length();
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: OsaOraIoTDevice\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + n + "\r\n\r\n" +
               connection);//login action =0
    DEBUG_PRINTLN("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      DEBUG_PRINTLN("headers received");
      break;
    }
  }
  String line = client.readStringUntil('#');
  DEBUG_PRINTLN(line);
  if (line.startsWith("s:")) { //s: = success
    sectetcode = String(line.substring(2));
    DEBUG_PRINT("secret code:");
    DEBUG_PRINTLN(sectetcode);
    line = client.readStringUntil('#');
    DEBUG_PRINT("Loop Time:");
    loopWaitTime=line.toInt()*1000;
    DEBUG_PRINTLN(loopWaitTime);
    line = client.readStringUntil('#');
    DEBUG_PRINT("Update Interval:");
    loopThreshold=line.toInt();
    loopCounter=loopCounter-1; //give it one round for 1st data packet
    DEBUG_PRINTLN(loopThreshold);
    line = client.readStringUntil('#');
    DEBUG_PRINT("High Alert Threshold 1:");
    highAlert1=line.toInt();
    DEBUG_PRINTLN(highAlert1);
    line = client.readStringUntil('#');
    DEBUG_PRINT("High Alert Threshold 2:");
    highAlert2=line.toInt();
    DEBUG_PRINTLN(highAlert2);
    line = client.readStringUntil('#');
    DEBUG_PRINT("Low Alert Threshold 1:");
    lowAlert1=line.toInt();
    DEBUG_PRINTLN(lowAlert1);
    line = client.readStringUntil('#');
    DEBUG_PRINT("Low Alert Threshold 2:");
    lowAlert2=line.toInt();
    DEBUG_PRINTLN(lowAlert2);
    line = client.readStringUntil('#');
    DEBUG_PRINT("device_status 1 :");
    device_status=line.toInt();
    DEBUG_PRINTLN(device_status);
    line = client.readStringUntil('#');
    DEBUG_PRINT("device_status 2 :");
    device_status2=line.toInt();
    DEBUG_PRINTLN(device_status2);
    line = client.readStringUntil('#');
    DEBUG_PRINT("Smart Rules 1 :");
    smartRules1=line.toInt();
    DEBUG_PRINTLN(smartRules1);
    line = client.readStringUntil('#');
    DEBUG_PRINT("Smart Rules 2 :");
    smartRules2=line.toInt();
    DEBUG_PRINTLN(smartRules2);    
    line = client.readStringUntil('\n');
    DEBUG_PRINT("Server Version:");
    serverVersion=line.toInt();
    DEBUG_PRINTLN(serverVersion);
    loginSuccess = true;
    if(serverVersion>currentVersion){
      getNewVersion();
    }else{
      DEBUG_PRINT("Current Version is similar to Server Version: ");
      DEBUG_PRINTLN(currentVersion);    
    }
    //switch on/off connected devices as per server status
    setupConnectedDevices();  
  } else {
    DEBUG_PRINT("Login failed!");
    //showValues(false);
    action = 0; //relogin plz ...
    //digitalWrite(ledPin, LOW);
  }
  client.stop();
  return sectetcode;
}
//This is OTA firmware upgrade method
//It has an issue with exsiting firmware as it is over http not https which need to be fixed for securing the values going 
//to the backend for proper OTA upgrade.
void getNewVersion(){
    DEBUG_PRINT("Getting New Version: ");
    DEBUG_PRINT(serverVersion);
    DEBUG_PRINT(" Current Version: ");
    DEBUG_PRINTLN(currentVersion);
    ESPhttpUpdate.rebootOnUpdate(true);
    //String encoded = base64::encode("OsaOra");
    String connection = "http://"+hostStr+":"+httpStr+"/IoTHub/IoTOTAServlet?i=" + base64::encode(deviceId) + "&p=" + base64::encode(devicepassword)+"&w="+ base64::encode(ssid) +"&wp="+base64::encode(password)+"&m="+deviceModel;
    //char* url; 
    //connection.toCharArray(url, connection.length());
    // t_httpUpdate_return ret = ESPhttpUpdate.update(host, httpsPort, connection, ""+currentVersion, true, fingerprint, true);
    // t_httpUpdate_return ret = ESPhttpUpdate.update(host, 8080, connection, ""+currentVersion);
    // t_httpUpdate_return ret = ESPhttpUpdate.update(client, host, url);
    // t_httpUpdate_return ret = ESPhttpUpdate.update(String(host),httpsPort,connection,String(""),String(fingerprint));
    t_httpUpdate_return ret = ESPhttpUpdate.update(connection);
    switch(ret) {
        case HTTP_UPDATE_FAILED:
            DEBUG_PRINTLN("[update] Update failed.");
            DEBUG_PRINTLN(ESPhttpUpdate.getLastError());
            DEBUG_PRINTLN(ESPhttpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            DEBUG_PRINTLN("[update] No Update Available.");
            break;
        case HTTP_UPDATE_OK:
            DEBUG_PRINTLN("[update] Update is Okay"); // will not be called as this will reboot the ESP
            ESP.restart();
            break;
    }
}
//This method send update message to the backend service
int sendUpdateMessage(String payload, int type) {
  DEBUG_PRINTLN("========== Update Message ===========");
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  DEBUG_PRINTLN("connecting using https port");
  DEBUG_PRINTLN(host);
  DEBUG_PRINTLN(httpsPort);
  if (!client.connect(host, httpsPort)) {
    DEBUG_PRINTLN("connection failed");
    //digitalWrite(ledPin, LOW);
    return 0;    
  }
  if (client.verify(fingerprint, host)) {
    DEBUG_PRINTLN("certificate matches");
  } else {
    DEBUG_PRINTLN("certificate doesn't match");
    //digitalWrite(ledPin, LOW);
    //return 0;    
  }
  String connection = "t=" + String(type) + "&a=5&i=" + base64::encode(deviceId) + "&s=" + sectetcode + "&m=" + payload;
  DEBUG_PRINTLN(connection);
  int n = connection.length();
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: OsaOraIoTDevice\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + n + "\r\n\r\n" +
               connection);
  String line = processResponse(client);
  if (line.startsWith("s:")) { //s: = success
    DEBUG_PRINTLN("Received action:");
    action = String(line.substring(2)).toInt();
    DEBUG_PRINT("Action:");
    DEBUG_PRINTLN(action);
    //digitalWrite(ledPin, HIGH);
    return action;
  } else {
    DEBUG_PRINTLN("Update message Error!!!");
    //digitalWrite(ledPin, LOW);
    return 0;
  }
}
//This methid ping the backend service for keeping alive the device
int sendPingMessage(String payload, int type) {
  DEBUG_PRINTLN("========== Ping ===========");
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  DEBUG_PRINTLN("connecting using https port");
  DEBUG_PRINTLN(host);
  if (!client.connect(host, httpsPort)) {
    DEBUG_PRINTLN("connection failed");
    //digitalWrite(ledPin, LOW);
    return 0;    
  }
  if (client.verify(fingerprint, host)) {
    DEBUG_PRINTLN("certificate matches");
  } else {
    DEBUG_PRINTLN("certificate doesn't match");
    //digitalWrite(ledPin, LOW);
    //return 0;    
  }
  String connection = "t=" + String(type) + "&a=3&i=" + base64::encode(deviceId) + "&s=" + sectetcode;
  DEBUG_PRINTLN(connection);
  int n = connection.length();
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: OsaOraIoTDevice\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + n + "\r\n\r\n" +
               connection);
  String line = processResponse(client);
  if (line.startsWith("s:")) { //s: = success
    DEBUG_PRINTLN("Received action:");
    action = String(line.substring(2)).toInt();
    DEBUG_PRINT("Action:");
    DEBUG_PRINTLN(action);
    //digitalWrite(ledPin, HIGH);
    return action;
  } else {
    DEBUG_PRINTLN("Ping Error!!!");
    //digitalWrite(ledPin, LOW);
    return 0;
  }
}
//This method process the response coming from ping/update methods.
String processResponse(WiFiClientSecure client) {
  DEBUG_PRINTLN("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      DEBUG_PRINTLN("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  DEBUG_PRINT("response body recieved, reply was:");
  DEBUG_PRINTLN(line);
  client.stop();
  return line;
}
//This method initialize the relay modules status as per the backend status
void setupConnectedDevices(){
  if(connectedDevices>0){
    if (device_status == 1) {
      //switch it on
      digitalWrite(RELAY_PIN, LOW); // Turns ON Relay 1
      DEBUG_PRINTLN("Turn ON Device 1");
    } else {
      //switch it off
      digitalWrite(RELAY_PIN, HIGH); // Turns OFF Relay 1
      DEBUG_PRINTLN("Turn OFF Device 1");
    }
    if(connectedDevices>1){
      if (device_status2 == 1) {
        //switch it on
        digitalWrite(RELAY_PIN2, LOW); // Turns ON Relay 2
        DEBUG_PRINTLN("Turn ON Device 2");
      } else {
        //switch it off
        digitalWrite(RELAY_PIN2, HIGH); // Turns OFF Relay 2
        DEBUG_PRINTLN("Turn OFF Device 2");
      }
    }
  }
}
//This method exeucte the high alert rules
void executeRulesHighAlert(){
  if(smartRules1==1 || smartRules1==11 || smartRules1==21){//switch on device 1 on high alert
    device_status = 1;
  }
  if(smartRules1==2 || smartRules1==12 || smartRules1==22){//switch off device 1 on high alert
    device_status = 0;
  }
  if(smartRules2==1 || smartRules2==11 || smartRules2==21){//switch on device 2 on high alert
    device_status2 = 1;
  } 
  if(smartRules1==2 || smartRules1==12 || smartRules1==22){//switch off device 2 on high alert
    device_status2 = 0;
  } 
  setupConnectedDevices();
}
//This method executes the low alert rules
void executeRulesLowAlert(){
  if(smartRules1==10 || smartRules1==11 || smartRules1==12){//switch on device 1 on low alert
    device_status = 1;
  }
  if(smartRules1==20 || smartRules1==21 || smartRules1==22){//switch off device 1 on low alert
    device_status = 0;
  }
  if(smartRules2==10 || smartRules2==11 || smartRules2==12){//switch on device 2 on low alert
    device_status2 = 1;
  } 
  if(smartRules1==20 || smartRules1==21 || smartRules1==22){//switch off device 2 on low alert
    device_status2 = 0;
  } 
  setupConnectedDevices();
}
//This is a simple method to play an alarm locally in the device
void playTone(){
  DEBUG_PRINTLN("========== Local Alarm ===========");
  int x=0;
  while(x<8){
    tone(BUZZER_PIN, 3000,500); 
    delay(1000);
    x++;
  }
  noTone(BUZZER_PIN);
  DEBUG_PRINTLN("========== End Local Alarm ===========");    
}
//get value from a separator and index
String getValue(String data, char separator, int index){
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//process the input value from the serial
//6 possible values: 
//wifi=x
//password=x
//id=device id
//pass=device pass
//clear=true (true or any other values) to restore the device default values
//restart=true (true or any other values) to restart the device
//TODO: all debug should be replaced by serial print statements
void updateVariables(String input){
  String key = getValue(input, '=', 0);
  String val = getValue(input, '=', 1);
  if (key!="" && val != "") {
    if(key=="wifi"){
      saveToMemory(10,val,val.length());
      Serial.print("Set new Wifi key [");
      Serial.print(val);
      Serial.println("], restart is required!");
    }else if(key=="password"){
      saveToMemory(60,val,val.length());
      Serial.print("Set new Wifi password [");
      Serial.print(val);
      Serial.println("], restart is required!");
    }else if(key=="ip"){
      if(WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi NOT connected");
      }else{
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());        
      }
    }else if(key=="id"){
      saveToMemory(110,val,val.length());
      Serial.print("Set new ID for the device [");
      Serial.print(val);
      Serial.println("], restart is required!");
    }else if(key=="pass"){
      saveToMemory(160,val,val.length());
      Serial.print("Set new device password [");
      Serial.print(val);
      Serial.println("], restart is required!");
    }else if(key=="clear"){
      clearValues();
      Serial.println("Restore Device Default Configuration!, restart is required!");
    }else if(key=="reset"){
      clearValues();
      saveToMemory(0,"NOO",3);
      Serial.println("Restore Factory Settings!, restart will follow!");
      ESP.restart();
    }else if(key=="list"){
      if(val==SHOW_PASSWORD_KEY) showValues(false);//show password key is used
      else showValues(true);
    }else if(key=="help"){
      Serial.println("===========================");
      Serial.println("Usage:");
      Serial.println("help=x: To show help");
      Serial.println("list=x: To show all cuurent configurations");
      Serial.println("clear=x: To restore default configurations");
      Serial.println("reset=x: To reset factory setting");
      Serial.println("ip=x: To show the IP Address of this device");
      Serial.println("id=x: To setup new device Id for the device");
      Serial.println("pass=x: To setup new device password for the device");
      Serial.println("wifi=x: To setup new wifi ssid");
      Serial.println("password=x: To setup new wifi password");
      Serial.println("restart=x: To restart the device so new configurations can be in place");
      Serial.println("===========================");
    }else if(key=="restart"){
      Serial.println("Wifi Restart!");
      //this means the user wants to proceed with the current configurations
      saveToMemory(0,"IOT",3);
      ESP.restart();     
    }else{
      Serial.println("Command Not Recognized!");
    }
  }
}
//clear existing values from the simulated memory
void clearValues(){
  saveToMemory(10,"",0);
  saveToMemory(60,"",0);
  saveToMemory(110,"",0);
  saveToMemory(160,"",0);
}
//display the configurations
//passwords either displayed or hidden as per the parameter
void showValues(boolean hidePasswords){
    String temp=readFromMemory(10);
    Serial.println("===========================");
    Serial.print("WiFi=");
    if(temp!="") {
      Serial.println(temp); 
    }else {
      Serial.print("[Default]=");
      Serial.println(ssidStr);
    }
    temp=readFromMemory(60);
    Serial.print("WiFi Password=");
    if(temp!="") {
      //TODO: should be decoded
      if(hidePasswords) Serial.println("*****");
      else Serial.println(temp);
    }else{
      Serial.print("[Default]=");
      if(hidePasswords) Serial.println("*****");
      else Serial.println(passwordStr);
    }
    temp=readFromMemory(110);
    Serial.print("Device Id=");
    if(temp!="") {
      Serial.println(temp);
    }else{
      Serial.print("[Default]=");
      Serial.println(deviceId);
    }
    temp=readFromMemory(160);
    Serial.print("Device Pass=");
    if(temp!="") {
      if(hidePasswords) Serial.println("*****");
      else Serial.println(temp); 
    }else{
      Serial.print("[Default]=");
      if(hidePasswords) Serial.println("*****");
      else Serial.println(devicepassword);
    }
    Serial.println("===========================");
}
//save a string value to an index in the EEPROM simulation memory
void saveToMemory(int address, String val,int len) {
  EEPROM.write(address, len);
  for(int i=0;i<len;i++){
    EEPROM.write(address+i+1, val.charAt(i));
  }
  EEPROM.commit();
}
//read a string value from an index in the EEPROM simulation memory
String readFromMemory(int address) {
  String output="";
  int length=EEPROM.read(address);
  if(length!=0){
    for(int i=0;i<length;i++){
      int value=EEPROM.read(address+i+1);
      output=output+char(value);
    }
  }
  return output;
}
//this method check if there is any incoming data from connected serial
void checkSerialInput(){
  if(Serial.available()) {
    String input = Serial.readString();
    input.trim();
    Serial.print("Command: ");
    Serial.println(input);
    updateVariables(input);
  }  
}
