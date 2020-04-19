
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
int THRESHOLD = 2;

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

BLYNK_WRITE(V3) 
{   
  setTemperature = param.asInt();
}

BLYNK_WRITE(V4) // Button Widget On Conditioner
{ 
  if(param.asInt() == 1) {     // if Button sends 1
    if(conditionState == 0) {
      conditionerAction(true);
    }
  } else {
    if(conditionState != 0 && climateState == 0) {
      conditionerAction(false);
//      Blynk.virtualWrite(V5, 0); 
    }
  }
}

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
  }
}

void myTimerEvent()
{ 
  Blynk.syncAll();
  if(setTemperature == 0 || tempMin == 0) {
    return;
  }
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
  Serial.println(climateState);
  

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

  if ((sensorData <= float(tempMin) + THRESHOLD) && (sensorData >= float(tempMin))) {
    Serial.println("SKIP");
    return;
  }
    
  // Turn On by min temp
  if (sensorData <= float(tempMin) && conditionState == 0) {
    Serial.print("CURRENT_TEMP < MIN --- ");
    Serial.println(tempMin);
    conditionerAction(true);
  }

  // Turn Off by max temp
  if (sensorData >= float(tempMin) + THRESHOLD && conditionState == 1) {
    Serial.print("CURRENT_TEMP > MAX --- ");
    Serial.println(tempMin + THRESHOLD);
    conditionerAction(false);
  }
}

void conditionerAction(bool action)
{ 
  if(action == true) {
    conditionState = 1;
    Blynk.virtualWrite(V4, 1);
  } else {
    conditionState = 0;
    Blynk.virtualWrite(V4, 0);
  }
  ac.setTemp(setTemperature);
  ac.setMode(AUTO_MODE);
  ac.setFan(FAN_AUTO);
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
  timer.setInterval(1000L, myTimerEvent);
  conditionerAction(false);
}

void loop() 
{
  Blynk.run();
  timer.run();
  delay(3000);
  
//#if SEND_COOLIX
  // Now send the IR signal.
//  ac.send();
//#else  // SEND_COOLIX
//  Serial.println("Can't send because SEND_COOLIX has been disabled.");
//#endif  // SEND_COOLIX
}
