
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SoftwareSerial.h>
#include <SimpleTimer.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Coolix.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define AUTO_MODE kCoolixAuto
#define COOL_MODE kCoolixCool
#define DRY_MODE kCoolixDry
#define HEAT_MODE kCoolixHeat
#define FAN_MODE kCoolixFan

#define FAN_AUTO kCoolixFanAuto
#define FAN_MIN kCoolixFanMin
#define FAN_MED kCoolixFanMed
#define FAN_HI kCoolixFanMax

// GPIO where the DS18B20 is connected to
const int oneWireBus = D1;

int tempMin = 0;
int setTemperature = 0;
int tempSave = 0;
int conditionState = 0;
int climateState = 0;
int isActive = 0;
int THRESHOLD = 1;
int condMode = 0;

char auth[] = "sUbjhUyB35sGgUhK_GHVPc3FSsUGKgnG";
const char *ssid =  "Xiaomi_236D";
const char *pass =  "ellisfromlos";

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRCoolixAC ac(kIrLed);  // Set the GPIO to be used to sending the message.
BlynkTimer timer;

WiFiClient client;

// Setup a oneWire instance to communicate with any OneWire devices
// Pass our oneWire reference to Dallas Temperature sensor // Pass our oneWire reference to Dallas Temperature sensor 
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

BLYNK_WRITE(V1) 
{   
  tempMin = param.asInt();
}

// Possible to set temperature manually from widget
//BLYNK_WRITE(V3) 
//{   
//  setTemperature = param.asInt();
//}

BLYNK_WRITE(V5) // Button Widget On Climate control
{ 
  if(param.asInt() == 1) {
    climateState = 1;
//    if(conditionState == 0) {
//      conditionerAction(true);
//      Blynk.virtualWrite(V4, 1);
//    }
  } else {
    climateState = 0;
    isActive = 0;
    if(conditionState == 1) {
      conditionerAction(false);
    }
  }
}

BLYNK_WRITE(V6) // Button Widget Mode (cool or heat)
{ 
  if(param.asInt() == 1) {
    // COOL
    condMode = 0;
    setTemperature = 22;
  } else {
    // HEAT
    condMode = 1;
    setTemperature = 30;
  }
}

void myTimerEvent()
{ 
  Blynk.syncAll();
//  if(setTemperature == 0 || tempMin == 0) {
//    return;
//  }
  sensors.requestTemperatures();
  float sensorData = sensors.getTempCByIndex(0);
  Blynk.virtualWrite(V0, sensorData);

  Serial.print("SENSOR: ");
  Serial.print(sensorData);
  Serial.print(" | SET MIN: ");
  Serial.print(tempMin);
  Serial.print(" | SET TEMP: ");
  Serial.print(tempSave);
  Serial.print(" || COND: ");
  Serial.print(conditionState);
  Serial.print(" | CLIM: ");
  Serial.print(climateState);
  Serial.print(" | ACTIVE: ");
  Serial.println(isActive);
  

  // Set temp
  if (tempSave != setTemperature && conditionState == 1) {
    ac.setTemp(setTemperature);
    ac.send();
    Serial.print("SET TEMP: ");
    Serial.println(setTemperature);
  }
  tempSave = setTemperature;

  if (climateState == 0) {
    Serial.println("Climate is OFF");
    return;
  }

  // HEAT
  if (condMode == 1) {
     
    if ((sensorData <= float(tempMin) + THRESHOLD) && (sensorData >= float(tempMin))) {
      Serial.println("SKIP");
      return;
    }
      
    // Turn On by min temp
    //  && conditionState == 0
    if (sensorData <= float(tempMin) && climateState == 1 && isActive == 0) {
      Serial.print("CURRENT_TEMP < MIN --- ");
      Serial.println(tempMin);
      conditionerAction(true);
      isActive = 1;
      Serial.println("TURN ON");
    }
  
    // Turn Off by max temp
    if (sensorData >= float(tempMin) + THRESHOLD && conditionState == 1 && isActive == 1) {
      Serial.print("CURRENT_TEMP > MAX --- ");
      Serial.println(tempMin + THRESHOLD);
      conditionerAction(false);
      isActive = 0;
      Serial.println("TURN OFF");
    }

  // COOL
  } else {

    if ((sensorData <= float(tempMin)) && (sensorData >= float(tempMin) - THRESHOLD)) {
      Serial.println("SKIP");
      return;
    }
      
    // Turn On by max temp
    //  && conditionState == 0
    if (sensorData >= float(tempMin) && climateState == 1 && isActive == 0) {
      Serial.print("CURRENT_TEMP < MIN --- ");
      Serial.println(tempMin);
      conditionerAction(true);
      isActive = 1;
      Serial.println("TURN ON");
    }
  
    // Turn Off by min temp
    if (sensorData <= float(tempMin) - THRESHOLD && conditionState == 1 && isActive == 1) {
      Serial.print("CURRENT_TEMP > MAX --- ");
      Serial.println(tempMin + THRESHOLD);
      conditionerAction(false);
      isActive = 0;
      Serial.println("TURN OFF");
    }
  }
  Serial.println("END");
}

void conditionerAction(bool action)
{ 
  if(action == true) {
    conditionState = 1;
//    Blynk.virtualWrite(V4, 1);
  } else {
    conditionState = 0;
//    Blynk.virtualWrite(V4, 0);
  }
  ac.setTemp(setTemperature);
  if (condMode == 1) {
    ac.setMode(HEAT_MODE);
  } else {
    ac.setMode(COOL_MODE);
    ac.setMode(FAN_MIN);
  }
//  ac.setFan(FAN_AUTO);
  ac.setPower(action);
  ac.send();
}

void setup() 
{
  ac.begin();
  // Start the DS18B20 sensor
  sensors.begin();
  Serial.begin(115200);
  delay(10);

  Serial.println("Connecting to ");
  Serial.println(ssid); 
  
  WiFi.begin(ssid, pass); 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.println("");
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(2000L, myTimerEvent);
  conditionerAction(false);
}

void loop() 
{
  if (Blynk.connected()) {  
  Blynk.run();  
  }
  else {
//  Blynk.connectWifi(ssid, pass);
    WiFi.begin(ssid, pass); 
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.println("");
      Serial.print(".");
    }
    Blynk.config(auth);
    Blynk.connect();
  }
  timer.run();
  delay(3000);
  
//#if SEND_COOLIX
  // Now send the IR signal.
//  ac.send();
//#else  // SEND_COOLIX
//  Serial.println("Can't send because SEND_COOLIX has been disabled.");
//#endif  // SEND_COOLIX
}
