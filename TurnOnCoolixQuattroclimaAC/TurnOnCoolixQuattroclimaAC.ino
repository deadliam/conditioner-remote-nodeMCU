/* Copyright 2017, 2018 crankyoldgit
* An IR LED circuit *MUST* be connected to the ESP8266 on a pin
* as specified by kIrLed below.
*
* TL;DR: The IR LED needs to be driven by a transistor for a good result.
*
* Suggested circuit:
*     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
*
* Common mistakes & tips:
*   * Don't just connect the IR LED directly to the pin, it won't
*     have enough current to drive the IR LED effectively.
*   * Make sure you have the IR LED polarity correct.
*     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
*   * Typical digital camera/phones can be used to see if the IR LED is flashed.
*     Replace the IR LED with a normal LED if you don't have a digital camera
*     when debugging.
*   * Avoid using the following pins unless you really know what you are doing:
*     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
*     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
*     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
*   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
*     for your first time. e.g. ESP-12 etc.
*/
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

// GPIO where the DS18B20 is connected to
const int oneWireBus = D1;

int lastState = 1;
int tempMin;
int tempMax;
int setTemperature;
int tempSave;
int onOffState = 0;

char auth[] = "sUbjhUyB35sGgUhK_GHVPc3FSsUGKgnG";
const char *ssid =  "Xiaomi_236D";
const char *pass =  "ellisfromlos";

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRCoolixAC ac(kIrLed);  // Set the GPIO to be used to sending the message.
BlynkTimer timer;

WiFiClient client;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

BLYNK_WRITE(V1) 
{   
  tempMin = param.asInt();
}
BLYNK_WRITE(V2) 
{   
  tempMax = param.asInt();
}
BLYNK_WRITE(V3) 
{   
  setTemperature = param.asInt();
}
BLYNK_WRITE(V4) // Button Widget On Conditioner
{
  if(param.asInt() == 1) {     // if Button sends 1
    ac.setPower(true);
    ac.send();
    lastState = 1;
    onOffState = 1;
  }
}
BLYNK_WRITE(V5) // Button Widget Off Conditioner
{
  if(param.asInt() == 1) {     // if Button sends 1
    ac.setPower(false);
    ac.send();
    lastState = 0;
    onOffState = 0;
  }
}

void myTimerEvent()
{ 
  Blynk.syncAll();
  float sensorData = sensors.getTempCByIndex(0);
  Serial.print("Temp: ");
  Serial.println(sensorData);
  Blynk.virtualWrite(V0, sensorData);
  if (tempSave != setTemperature)
  {
//    Serial.print("SET: ");
//    Serial.println(setTemperature);
    ac.setTemp(setTemperature);
    ac.send();
  }
  if ((sensorData > tempMin && sensorData < tempMax) || onOffState == 0)
  {
    return;  
  }

  if (sensorData <= tempMin && lastState == 0)
  {
    Serial.println("temp < MIN");
    Serial.println(tempMin);
    ac.setPower(true);
    ac.send();
//    ac.setFan(kCoolixFanAuto);
//    ac.setMode(kCoolixAuto);
    lastState = 1;
  } 
  if (sensorData >= tempMax && lastState == 1)
  {
    Serial.println("temp > MAX");
    Serial.println(tempMax);
    ac.setPower(false);
    ac.send();
    lastState = 0;
  }
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
}

void loop() 
{
  Serial.print("MIN: ");
  Serial.print(tempMin);
  Serial.print(" | MAX: ");
  Serial.println(tempMax);
  
  tempSave = setTemperature;
  sensors.requestTemperatures();
  Blynk.run();
  timer.run();
  delay(5000);
  
//#if SEND_COOLIX
  // Now send the IR signal.
//  ac.send();
//#else  // SEND_COOLIX
//  Serial.println("Can't send because SEND_COOLIX has been disabled.");
//#endif  // SEND_COOLIX
}
