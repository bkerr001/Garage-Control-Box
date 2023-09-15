#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <homespan.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "DEV_HeaterCooler.h"
#define I2C_SDA 20
#define I2C_SCL 19

TwoWire I2CBME = TwoWire(0); // TwoWire for BME 280
Adafruit_BME280 bme;         // I2C

struct DEV_TempSensor : Service::TemperatureSensor
{ // A standalone Temperature sensor

  SpanCharacteristic *temp; // reference to the Current Temperature Characteristic

  DEV_TempSensor() : Service::TemperatureSensor()
  { // constructor() method

    double tempC = bme.readTemperature();

    temp = new Characteristic::CurrentTemperature(tempC); // instantiate the Current Temperature Characteristic
    temp->setRange(-50, 100);                             // expand the range from the HAP default of 0-100 to -50 to 100 to allow for negative temperatures

    Serial.print("Configuring Temperature Sensor"); // initialization message
    Serial.print("\n");

  } // end constructor

  void loop()
  {

    if (temp->timeVal() > 5000)
    {                                       // check time elapsed since last update and proceed only if greater than 5 seconds
      double tempC = bme.readTemperature(); // "simulate" a half-degree temperature change...
      double getval = temp->getVal<float>();
      Serial.print("getVal: ");
      Serial.println(getval);
      if (abs(getval - tempC) > 0.5)
      {
        temp->setVal(tempC);
        Serial.println("New Value Set");
      }
    }
  }
};

struct DEV_HeaterCooler : Service::HeaterCooler
{
  int heaterpin;
  // Create characteristics, set initial values, and set storage in NVS to true

  SpanCharacteristic *active;
  SpanCharacteristic *currentState;
  SpanCharacteristic *targetState;
  SpanCharacteristic *currentTemp;
  SpanCharacteristic *heatingThreshold;
  SpanCharacteristic *coolingThreshold;
  SpanCharacteristic *displayUnits;
  DEV_HeaterCooler(int heaterpin) : Service::HeaterCooler()
  {
    double tempC = bme.readTemperature();
    active = new Characteristic::Active(0, true);
    this->heaterpin = heaterpin;
    pinMode(heaterpin, OUTPUT);

    currentState = new Characteristic::CurrentHeaterCoolerState(0, true);
    targetState = new Characteristic::TargetHeaterCoolerState(0, true);
    currentTemp = new Characteristic::CurrentTemperature(tempC, true);
    currentTemp->setRange(-50, 100);
    heatingThreshold = new Characteristic::HeatingThresholdTemperature(22, true);
    coolingThreshold = new Characteristic::CoolingThresholdTemperature{22, true};
    displayUnits = new Characteristic::TemperatureDisplayUnits(0, true);
