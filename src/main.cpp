/*

/********************************************************
  Somoke House Version 1.0 
******************************************************


  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

// Import required libraries
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <AsyncElegantOTA.h>
#include <OneWire.h>            // Library for the DS18B20 sensor
#include <DallasTemperature.h>  // Library for the DS18B20 sensor
#include <PID_v1.h>             // Library for PID 
//#include <U8g2lib.h>            // Library for display

#include <SetupSpiffs.h> // Internal library




/*****************************************************
  WIFI - MODES
******************************************************/

const int WiFiMode = 0;   // 0 - Hardcoded STA MODE, 1 - Hardcoded AP Mode, 2 - WifiManager

/*****************************************************
  WIFI - HARDCODED
******************************************************/

// Replace with your network credentials    //hardcoded

const char* ssid_STA = "REPLACE_WITH_YOUR_SSID";
const char* password_STA = "REPLACE_WITH_YOUR_PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

/*****************************************************
  WIFI - MANAGER
******************************************************/

// Search for parameter in HTTP POST request  **WIFIMANAGER**
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";


//Variables to save values from HTML form   **WIFIMANAGER**
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently  **WIFIMANAGER**
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded     **WIFIMANAGER**

// Set your Gateway IP address        **WIFIMANAGER**
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded    **WIFIMANAGER**
IPAddress subnet(255, 255, 0, 0);

// Timer variables       **WIFIMANAGER**
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)
#include <WifiMgr.h>

/*****************************************************
  INPUT FIELDS
******************************************************/

String SetTempMain;                  // Temperatrure of Smnoker
String message = "";                 // Handle messages from Client


/*****************************************************
  OUTPUTS - GPIOS / SOFT
******************************************************/

// Set number of outputs
#define NUM_OUTPUTS  6

// Assign each GPIO to an output
int outputGPIOs[NUM_OUTPUTS] = {18, 19, 12, 14, 40, 41};
int NumSoftOutputs = 2;
// int SoftoutputGPIOs[NUM_SOFTOUTPUTS] = {40,41};

/*****************************************************
  CONTROL VARIABLES
******************************************************/

// Variables for SoftOutputs

bool PidRun = 0;   // GPIO 40 PID ON/OFF 0 - Stopped, 1 -Run
bool ProgTimer = 0; // GPIO41
// int ProgVariable = 0;

// Control parameters - IDLE, RUN, STOP, ALARM, OVERHEAT, SENSOR FAULT

bool Sensor1Fault = 0;     // 0 - sensor OK, 1 - sensor not found
String TempMaxMain = "95";          // Max allowed temperature
bool OverHeat = 0;
int TempOverheat = 115; 
// bool RunState = 0;

// ***** Messages *****

String StatusMsg;
String PowerMsg;

bool hasRun = 0;
bool hasRun1 =0;

/*****************************************************
  PID - SETUP
******************************************************/

// GPIO where the Heater SSR is connected to **Sensor Readings**
const byte RelayPin = 27;
const bool RELAY_PIN_ACTIVE_LEVEL = LOW;  // LOW for ON, HIGH for OFF     

//Define input/output variables for PID
double Setpoint , Input;  // Temperature (must be in the same units)
double Output;  // 0-WindowSize, Part of PWM window where output is ACTIVE.
int Power; // Heater Power indicator

//Define the aggressive and conservative Tuning Parameters
double aggKp=50, aggKi=0.4, aggKd=10;
double consKp=60, consKi=0.7, consKd=10;

//Specify the links and initial tuning parameters
//double Kp = 60; // Proportional Constant: Active 10 milliseconds for each degree low
//double Ki = 0.5;  // Integral Constant: Increase to prevent offset (Input settles too high or too low)
//double Kd = 10;  // Differential Constant:  Increase to prevent overshoot (Input goes past Setpoint)

// Note: "DIRECT" means Higher Output -> Higher Input. (like a heater raises temperature)
//       "REVERSE" means Higher Output -> Lower Input. (like a cooler lowers temperature)
PID myPID(&Input, &Output, &Setpoint, consKp, consKi, consKd, DIRECT);

const float WindowSize = 5000.0;
unsigned long WindowStartTime = 0;
unsigned long currentTime = millis();



/*****************************************************
  READINGS - SETUP
******************************************************/

   // Create an Event Source on /events **Sensor Readings**
    AsyncEventSource events("/events");

    // Json Variable to Hold Sensor Readings **Sensor Readings**
    JSONVar readings;

    // Timer variables  **Sensor Readings**
    unsigned long lastTime = 0;
    unsigned long timerDelay = 5000;
   
    // GPIO where the DS18B20 is connected to **Sensor Readings**
    const int oneWireBus = 2;

    // Setup a oneWire instance to communicate with any OneWire devices **Sensor Readings**
    OneWire oneWire(oneWireBus);

    // Pass our oneWire reference to Dallas Temperature sensor **Sensor Readings**
    DallasTemperature sensors(&oneWire);
    //DeviceAddress sensor1;

    // Get Sensor Readings and return JSON object **Sensor Readings**
    String getSensorReadings(){

      if (PidRun == 1 & Sensor1Fault == 0) 
        readings["temperature"] = Input;
        else
        {
      sensors.requestTemperatures();
      readings["temperature"] = String(sensors.getTempCByIndex(0));
        }
      readings["setpoint"] = Setpoint;
      readings["power"] = PowerMsg; 
      readings["statusmsg"] = StatusMsg;
      //readings["humidity"] =  String(sensors.getTempCByIndex(0));

     String jsonString = JSON.stringify(readings);
      return jsonString;
    }

/*****************************************************
  WIFI - HARDCODED
******************************************************/

// Initialize WiFi STA MODE // **hard coded wifi**

void initWiFiSTA() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_STA, password_STA);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
  Serial.print('.');
  delay(1000);
  }
  Serial.println(WiFi.localIP());
}
// Initialize WiFi AP MODE   // **hard coded wifi AP**

 void initWiFiAP() {
  Serial.println("Setting AP (Access Point)");
  // NULL sets an open Access Point
  WiFi.softAP("SMOKE-HOUSE", NULL);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP); 
 }

 void notFound(AsyncWebServerRequest *request) {   //Input fields
  request->send(404, "text/plain", "Not found");
}

String getOutputStates(){
  JSONVar myArray;
  for (int i =0; i<NUM_OUTPUTS; i++){
    myArray["gpios"][i]["output"] = String(outputGPIOs[i]);
    if (i == NUM_OUTPUTS-1)
      myArray["gpios"][i]["state"] = ProgTimer;
    if (i == NUM_OUTPUTS - NumSoftOutputs)
      myArray["gpios"][i]["state"] = PidRun;
if (i < NUM_OUTPUTS - NumSoftOutputs) 
    myArray["gpios"][i]["state"] = String(digitalRead(outputGPIOs[i])); 
  }

 // for (int i =0; i<NUM_SOFTOUTPUTS; i++){
 //   if (i== 0) ProgVariable = ProgRun;
 //   if (i ==1) ProgVariable = ProgTimer; 
  //  myArray["softgpios"][i]["output"] = String(SoftoutputGPIOs[i]);
  //  myArray["softgpios"][i]["state"] = ProgVariable;
  //}
  String jsonString = JSON.stringify(myArray);
  return jsonString;
}

void notifyClients(String state) {
  //Serial.println(state); // *******Print status of GPIO's ******
  ws.textAll(state);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
   if (strcmp((char*)data, "states") == 0) {
      notifyClients(getOutputStates());
    }
    else{
      int gpio = atoi((char*)data);
      if (gpio < 39) 
      digitalWrite(gpio, !digitalRead(gpio));
      if (gpio == 40) PidRun = !PidRun;
      if (gpio == 41) ProgTimer = !ProgTimer;
      //Serial.print("ProgRun ");
      //Serial.print(PidRun);
      //Serial.print("ProgTimer ");
      //Serial.print(ProgTimer);
      //Serial.print("Gpio ");
      //Serial.print(gpio);
      notifyClients(getOutputStates());
    }
    
    message = (char*)data;
    if (message.substring(0, message.indexOf(".")) == "SetTempMain")
     {
    SetTempMain= message.substring(message.indexOf(".")+1, message.length());
    if (SetTempMain.toInt() >= TempMaxMain.toInt()) SetTempMain = TempMaxMain; 
    writeFile(SPIFFS, "/SetTempMain.txt", SetTempMain.c_str());
     }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}


/*****************************************************
  SYSTEM MESSAGES
******************************************************/

void SystemMsg() {

if (Sensor1Fault == 1|| OverHeat == 1 ) 
{ StatusMsg = "No DS18 Sensor";
if (OverHeat == 1) StatusMsg = "Overheat";
}
else
StatusMsg = "OK";

}




void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Set GPIOs as outputs
  for (int i =0; i<NUM_OUTPUTS; i++){
    pinMode(outputGPIOs[i], OUTPUT);
  }

  initSPIFFS();
  if (WiFiMode == 0) initWiFiSTA();   //   for hard coded WIFI STA
  if (WiFiMode == 1) initWiFiAP(); // for hard coded WIFI AP
  initWebSocket();



/*****************************************************
  WIFI - MANAGER
******************************************************/
if (WiFiMode == 2) {

// Load values saved in SPIFFS   **WIFIMANAGER**
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  if(initWiFi()) {     // **WIFIMANAGER**
    // Route for root / web page   
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false);
    });
    server.serveStatic("/", SPIFFS, "/");

// Request for the latest sensor readings **Sensor Readings**
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("SMOKE HOUSE", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL  **WIFIMANAGER**
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}


if (WiFiMode == 0 || WiFiMode == 1) {

  // Route for root / web page  * for hard coded WIFI*
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html",false);
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings **Sensor Readings**
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
 
  // Start server * for hard coded WIFI*
  server.begin();
}

// Start ElegantOTA
  AsyncElegantOTA.begin(&server);



/*****************************************************
  PID - INITIALS
******************************************************/

 //setup pin to act as output
  pinMode(RelayPin, OUTPUT);
  
  // Desired temperature
  Setpoint = 30.0;

  //tell the PID to range between 0 and the full window size
  myPID.SetOutputLimits(0, WindowSize);

  //turn the PID on
  myPID.SetMode(AUTOMATIC);
}

void loop() {

/*****************************************************
  PID - RUN
******************************************************/

sensors.requestTemperatures();
Input = sensors.getTempCByIndex(0);

/*****************************************************
  DS18B20 SENSOR CHECK
******************************************************/

if (Input == -127 || Input > TempOverheat )
{
 if (hasRun1 == 0) { //Run code one time
  PidRun = 0; 
  notifyClients(getOutputStates());
  
  if (Input == -127)  Sensor1Fault = 1; 
  if (Input > TempOverheat) OverHeat = 1;
    hasRun1 = 1;
  }}
  else
  {
 Sensor1Fault = 0;
 OverHeat = 0;
 hasRun1 = 0;
  }

if (PidRun == 1 & Sensor1Fault == 0) {

  if (hasRun == 0) { //Run code one time
  
  pinMode(RelayPin, OUTPUT);  //setup pin to act as output
  hasRun = 1;
  }

unsigned long currentTime = millis();

  //sensors.requestTemperatures();
  //Input = sensors.getTempCByIndex(0);
  
  /************************************************
     switch aggressive - conservative mode 
   ************************************************/

  double gap = abs(Setpoint-Input); //distance away from setpoint
  if (gap < 5)
  {  //we're close to setpoint, use conservative tuning parameters
    myPID.SetTunings(consKp, consKi, consKd);
  }
  else
  {
     //we're far from setpoint, use aggressive tuning parameters
     myPID.SetTunings(aggKp, aggKi, aggKd);
  }
 
  myPID.Compute();

  /************************************************
     turn the output pin on/off based on pid output
   ************************************************/
  if (currentTime - WindowStartTime > WindowSize)
  {
    //time to shift the Relay Window
    WindowStartTime += WindowSize;
  }

  // "Output" is the number of milliseconds in each
  // window to keep the output ACTIVE
  if (Output < (currentTime - WindowStartTime))

    digitalWrite(RelayPin,RELAY_PIN_ACTIVE_LEVEL);  // ON 
  else
    digitalWrite(RelayPin, !RELAY_PIN_ACTIVE_LEVEL);  // OFF 

Power = (Output * 100.0) / WindowSize;
PowerMsg = String (Power) + " %";
}
else 
{
PowerMsg = "Stopped";
digitalWrite(RelayPin, LOW);
hasRun = 0;
}


/*****************************************************
  READINGS VALUES SEND
******************************************************/

 if ((millis() - lastTime) > timerDelay) {
   
    // Send Events to the client with the Sensor Readings Every 10 seconds
    SystemMsg();
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
    
/*****************************************************
 LOAD STORED DATA - INPUT FILEDS
******************************************************/

// To access your stored values on inputString, inputInt, inputFloat
// String yourInputString = readFile(SPIFFS, "/inputString.txt");
//  int yourInputInt = readFile(SPIFFS, "/inputInt.txt").toInt();
Setpoint = readFile(SPIFFS, "/SetTempMain.txt").toInt();
//  float yourInputFloat = readFile(SPIFFS, "/inputFloat.txt").toFloat();

    /* Serial.print("Setpoint: ");
    Serial.print(Setpoint, 1);
    Serial.print("°C, Input:");
    Serial.print(Input, 1);
    Serial.print("°C, Output:");
    Serial.print((int)Output);
    Serial.print(" milliseconds (");
    Serial.print((Output * 100.0) / WindowSize, 0);
    Serial.println("%)"); */
  }
  
   ws.cleanupClients(); //swith status
}