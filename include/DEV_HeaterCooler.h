
//logic will be as follows
#include <Arduino.h>
#include "HomeSpan.h" 
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Wire.h>
#define I2C_SDA 20
#define I2C_SCL 19
#define MAXLEN 255
TwoWire I2CBME = TwoWire(0);//TwoWire for BME 280
Adafruit_BME280 bme; // I2C

struct DEV_HeaterCooler : Service::HeaterCooler {
  int heaterpin;
  int blowerpin;
  bool blowerstate;
  int setTime = 0;
  Characteristic::Active active {0, true};
  Characteristic::CurrentHeaterCoolerState currentState{0,true};
  Characteristic::TargetHeaterCoolerState targetState{0,true}; 
  Characteristic::CurrentTemperature currentTemp{22,true};
  Characteristic::HeatingThresholdTemperature heatingThreshold{22,true};
  Characteristic::CoolingThresholdTemperature coolingThreshold{22,true};  
  Characteristic::TemperatureDisplayUnits displayUnits{0,true};               // this is for changing the display on the actual thermostat (if any), NOT in the Home App
 
    DEV_HeaterCooler(int Heaterpin, int Blowerpin) : Service::HeaterCooler() {
      I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
      WEBLOG("BME280 test");
      unsigned status;
      status = bme.begin(0x76, &I2CBME); 
        double tempC= bme.readTemperature();
        currentTemp.setVal(tempC);
        currentTemp.setRange(-450,100);
        blowerpin = Blowerpin;
        heaterpin=Heaterpin;
        blowerstate=0;
        pinMode( heaterpin, OUTPUT);
        pinMode( blowerpin, OUTPUT);
        digitalWrite(heaterpin, LOW);
        digitalWrite(blowerpin, LOW);
    }

    boolean update() override {
        if(targetState.updated()){
            switch(targetState.getNewVal()){
            case 0:
                WEBLOG("Thermostat set to AUTO from %s to %s\n",temp2String(heatingThreshold.getVal<float>()).c_str(),temp2String(coolingThreshold.getVal<float>()).c_str());
                break;
            case 1:
                WEBLOG("Thermostat set to Heating Mode, returning to auto"); //I should turn this back to auto with heatingThreshould.
                break;
            case 2:
                WEBLOG("Thermostat set to cooling mode, this feature is not active, returning to auto mode."); // Add some new stuff here.
                break;
            }
        }
        if(heatingThreshold.updated() || coolingThreshold.updated())
            WEBLOG("Temperature range changed to %s to %s\n",temp2String(heatingThreshold.getNewVal<float>()).c_str(),temp2String(coolingThreshold.getNewVal<float>()).c_str());
        if(displayUnits.updated())
            WEBLOG("Display Units changed to %c\n",displayUnits.getNewVal()?'F':'C');

        
        return(true);
        }
    //!---------------Variables declared below this line-----------------------------
    String temp2String(float temp){
    String t = displayUnits.getVal()?String((temp*1.8+32.0)):String(temp);
    t+=displayUnits.getVal()?" F":" C";
    return(t);    
    }// close temp2string 

    float celcius(float temperature){
        float converted = (temperature*1.8) +32 ;
        return(converted);
    }//close celcius

     void printstats(){
        char buffer[255];
        float readTemp= bme.readTemperature();
        float tempF = celcius(readTemp);
        float currentTempF = celcius(currentTemp.getVal<float>());
        snprintf(buffer,MAXLEN,"Read Temp: %s C/ %s F   Current Temp:%sC/ %sF   Heating Temp: %sC/ %sF   Cooling Temp:  %s   Current State: %s   Target State: %s  Current State Time: %s  SetTime; %s",
          String(readTemp).c_str(),
          String(tempF).c_str(),
          String(currentTemp.getVal<float>()).c_str(),
          String(currentTempF).c_str(),
          String(heatingThreshold.getVal<float>()).c_str(),
          String(celcius(heatingThreshold.getVal<float>())).c_str(),
          String(coolingThreshold.getVal<float>()).c_str(),
          String(currentState.getVal()).c_str(),
          String(targetState.getVal()).c_str(),
          String(currentState.timeVal()).c_str(),
          String(setTime).c_str() ); // close sprintf
        WEBLOG(buffer);
        setTime = currentState.timeVal();
    } // Close printstats

    void heaterOn(){
        currentState.setVal(2); 
        digitalWrite(blowerpin, HIGH);
        delay (2000);//! Increased to 4 seconds as I moved the blower on top rather than inside. 
        digitalWrite(heaterpin, HIGH);
        blowerstate = 0;
    } //close heateron

    void heaterOff(){
        currentState.setVal(0);
        digitalWrite(heaterpin, LOW);
        digitalWrite(blowerpin, HIGH);
        blowerstate = 1;
    }//close heaterOn

    void blowerOff(){
        digitalWrite(blowerpin, LOW);
        blowerstate = 0;
    }// close blowerOff

    float checkforNewTemp(){
        float readTemp = bme.readTemperature();
        int errorFlag = 0;
        int prevState = currentState.getVal();//! I CAN PROBABLY MAKE THIS CLEANER. - IT WOUDLNT LET ME DEFINE IT IN THE ERRORFLAG=0 CLAUSE.
        while (isnan(readTemp) || readTemp == 0 ){
                while(errorFlag==0){
                    int prevState = currentState.getVal();//! I CAN PROBABLY MAKE THIS CLEANER. - IT WOUDLNT LET ME DEFINE IT IN THE ERRORFLAG=0 CLAUSE.
                    WEBLOG("NAN ERROR, (%s) Heater and Cooler being turned off for safety.",String(readTemp).c_str());
                    digitalWrite(heaterpin, LOW);
                    delay (2000);// increased delay here to 10 seconds. Should help debounce too.
                    digitalWrite(blowerpin, LOW);
                    currentState.setVal(0);
                    errorFlag=1;
                    WEBLOG("Error Flag = %s",String(errorFlag).c_str());
                }
                bme.begin(0x76, &I2CBME);
                delay(2000);
                readTemp = bme.readTemperature();
                delay(1000);
                if(isnan(readTemp) || readTemp == 0){
                    WEBLOG("Unable to sucessfully restart temp service, temp is reading %s", String(readTemp).c_str());
                }
                else {WEBLOG("The BME service was suceessfully restarted, the new Temp is reading %s", String(readTemp).c_str());
                    WEBLOG("Setting HeaterSate to previous state (%s)", String(currentState.getVal()).c_str());
                    currentState.setVal(prevState);
                    errorFlag =0;
                    printstats();
                    WEBLOG("Exiting Error State");}
        }// Close while bme.readTemperature is nan or 0
        if(abs(celcius(currentTemp.getVal<float>())-celcius(readTemp))>0.5){
            currentTemp.setVal(readTemp);
            WEBLOG("New Value Set (%s)C (%s)",String(readTemp).c_str(),String(celcius(readTemp)).c_str()); //! This is a value change
            printstats();
            return (readTemp);
        }
        else {float t = currentTemp.getVal<float>();
            return (t);
        }
    }// Close Check temp

//! -------------------------Loop below this line ----------------------------
    void loop() {
        if(active.getVal()==1) {
            if(currentTemp.timeVal()>10000 && targetState.getVal() == 0){
                float tempC = checkforNewTemp(); // check for a new temp? if there is no new temp..
                float diff= (coolingThreshold.getVal<float>()-heatingThreshold.getVal<float>())/2;                              
                if (currentState.timeVal() - setTime > 120000){
                    printstats();
                }
                switch(currentState.getVal()){
                    case 0: // if current state is Inactive: 
                        // First Check to make sure the heater is off.

                        if (blowerstate == 1 && digitalRead(blowerpin) == 1 && currentState.timeVal() > 60000){
                          blowerOff();
                        }// This is just a catchall to make sure that the blower turns off a few seconds after the heater turns off.
                        if(digitalRead(heaterpin)!= 0){
                            WEBLOG("Turning HEAT OFF, not sure why its on. \n");
                            heaterOff();
                        }// then check if the tempis lower then the heating threshold - if it is, turn the heater on.
                        else if (tempC < heatingThreshold.getVal<float>()){
                            WEBLOG("DEBUG: Turning Heater on because current state is %s, read temp (%s) is less than heating threshold(%S).\n", String(currentState.getVal()).c_str(), String(tempC).c_str(), String(heatingThreshold.getVal<float>()).c_str() );
                            heaterOn();
                        }
                    break;
                    case 1: // if the heater cooler state is idle, turn the heater off because it is an unkown state
                      if(digitalRead(heaterpin)!= 0){
                        heaterOff();
                        WEBLOG("DEBUG: Turning Heater on because current state is (%s).\n", String(currentState.getVal()).c_str() );
                        }
                    break;
                    case 2:// if the heater is in heating state.
                      if(digitalRead(heaterpin)!= 1 && tempC < (heatingThreshold.getVal<float>() +diff)){// make sure the heater is on, if it is not, turn it on. 
                          heaterOn();
                          WEBLOG("DEBUG: Turning Heater on because state is heating but heaterpin was off.\n");
                        }
                      else if(digitalRead(heaterpin) == 1 ){ // if the heater is on and the blower is on check to see if we are above the threshold
                          if(tempC > (heatingThreshold.getVal<float>() + diff)){
                              WEBLOG("DEBUG: Turning Heater off because temp (%s) above middle threshold(%s).\n", String(tempC).c_str(), String((heatingThreshold.getVal<float>()+diff)).c_str() );
                              heaterOff();                                                 // if we are turnt he heater off.
                            }
                      break;// dont need to process any further if this is the case. 
                      }
                      else if(digitalRead(heaterpin) == 1 && digitalRead(blowerpin) == 0){ // if the heater is on appropriatley and the blower is off, turn the blower on. 
                          if(tempC < (heatingThreshold.getVal<float>()+diff))
                          digitalWrite(blowerpin, HIGH); //! This might not be necessay - the only case I can think of is on startup where previous heaterstate was on. 
                          WEBLOG("DEBUG: Turning Blower on because heater was on and blower wasnt \n");
                      }
                        break; 
                    } //close switch
            } //close if currenttime
            else if(currentTemp.timeVal()>10000 && targetState.getVal() != 0){ 
                heaterOff();
                WEBLOG("DEBUG: Turning everything off because of unknown state..  Active but target state not auto \n");                
            }// I can expand this with a switch case heater - use heating threshold and heating threshold +4 to set heater and cooler targets.
        } //close if active
        else if(active.getVal()==0 && digitalRead(heaterpin)==HIGH){
        WEBLOG("Termostat is inactive, Turning Everything off! \n");

        heaterOff();
        delay(500);
        blowerOff();
        } // close else if
    //    else if (currentState.getVal() == 0 && currentState.timeVal() - setTime > 1200000)  { //! THis is new - just reading out a temp even in the case that the heater is off.
    //         setTime = currentState.getVal();
    //         float readTemp = bme.readTemperature();
    //         WEBLOG("Just updating the heat bruh");
    //         if(abs(celcius(currentTemp.getVal<float>())-celcius(readTemp))>0.5){
    //         currentTemp.setVal(readTemp);
    //         printstats();
    //     }         }              
} //close loop

}; // close struct
//!-------------------------Program Notes below this line----------------------------
/*
CurrentHeaterCoolerState:
  0= Inactive
  1= Idle - //! I do not use this state.
  2= The accessory is actively heating.
  4= The accessory is actively cooling.

TargetHeaterCoolerState:
  0= The accessory should choose whether to heat or cool. AKA Auto
  1= The accessory should operate as a heater.
  2= The accessory should operate as a cooler.

Could write something like if it is 1 or 2, change to zero.
*/