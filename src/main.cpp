#include <Arduino.h>
#include <homespan.h>
#include "DEV_HeaterCooler.h"
#include "DEV_LEDx.h"
#include "DEV_WindowBlinds.h"
#define SERIAL_BAUD 115200
unsigned long delayTime = 250;

void setup()
{
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  homeSpan.setLogLevel(1);
  homeSpan.enableOTA();
  homeSpan.setStatusPixel(38, 120, 100, 20);
  homeSpan.enableWebLog(100, "pool.ntp.org", "UTC+8:00", "myLog");
  homeSpan.begin(Category::Bridges, "WOAH Bridge");
  //!----------------------- Declare accessories here-------------
  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  new Characteristic::Name("Garage Lights");
  new DEV_LED(11);

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  new Characteristic::Name("Garage Acc 1");
  new DEV_LED(12);

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  new Characteristic::Name("Garage Heater");
  new DEV_HeaterCooler(10, 13);

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  new Characteristic::Name("Window Shade");
  new DEV_WindowShade(21);
  //!--------------------------------------------------------
}

void loop()
{
  homeSpan.poll();
  delay(delayTime);
}