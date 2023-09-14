#include <Arduino.h>
#include "HomeSpan.h"
#define  BIT_US 320

////////////////////////////////////

struct DEV_WindowShade : Service::WindowCovering {     // A motorized Window Shade with Hold Feature


    int txPin;
    const char * preamble = "1010101010101010101010101010101010101010101010101010101010101010101010101010101010101010";
    const char * header = "110011001100110011001100110011001111111100000000";
    const char * code = "" ;
    int interrupt = 0;
    float seconds = 1000;
    int dist =0;

  SpanCharacteristic *current;                     // reference to a "generic" Current Position Characteristic (used by a variety of different Service)
  SpanCharacteristic *target;                      // reference to a "generic" Target Position Characteristic (used by a variety of different Service)

  DEV_WindowShade( int TXPIN) : Service::WindowCovering(){       // constructor() method
    txPin = TXPIN;
    current=new Characteristic::CurrentPosition(0);     // Window Shades have positions that range from 0 (fully lowered) to 100 (fully raised)
    target=new Characteristic::TargetPosition(0);       // Window Shades have positions that range from 0 (fully lowered) to 100 (fully raised)
    target->setRange(0,100,10);                         // set the allowable target-position range to 0-100 IN STEPS of 10
    Serial.print("Configuring Motorized Window Shade");   // initialization message
    Serial.print("\n");
    pinMode (txPin, OUTPUT);
    Serial.println(txPin);

  } // end constructor

/// TRANSMIT FUNCITON
void do_transmit() {
  size_t preamble_size = strlen(preamble);
  size_t header_size = strlen(header);
  size_t code_size = strlen(code);
  WEBLOG("Preamble %s Header %s Code %s TxPin %s",String(preamble_size), String(header_size), String(code_size), String(txPin)   );

  int runcount =0;
    for (size_t i = 0; i < 20; i++) {
      for (size_t i = 0; i < preamble_size; i++) {
        digitalWrite(txPin, preamble[i] == '1' ? HIGH : LOW);
        delayMicroseconds(BIT_US);}
      }
    while(runcount<3){
        for (size_t i = 0; i < 1; i++) {
          for (size_t i = 0; i < header_size; i++) {
            digitalWrite(txPin, header[i] == '1' ? HIGH : LOW);
            delayMicroseconds(BIT_US);
          }

          for (size_t i = 0; i < code_size; i++) {
            digitalWrite(txPin, code[i] == '1' ? HIGH : LOW);
            delayMicroseconds(BIT_US);
          }
        }
        runcount++;
      }

}


void lower(){
WEBLOG("Target Newval %s",String(target->getNewVal()));
WEBLOG("Target GetVal %s",String(target->getVal()));
WEBLOG("CurrentGetVal %s",String(current->getVal()));
code = "11000011001100110011001100110011001100111100001100111100001100111100110000110011001100110011001111001100110000110011001100110011001100111100110000111100001100110011001100";
if (target->getNewVal() == 0){
    do_transmit();}
else {
  dist = ((current->getVal()-target->getNewVal())/10) * seconds;
  do_transmit();
  WEBLOG("Transmitted LOWER %s %s \n", code, String(dist));
}
};

void raise(){
code = "11000011001100110011001100110011001100111100001100111100001100111100110000110011001100110011001111001100001100110011001100110011001100110011110000111100001100110011001100";
WEBLOG("Target Newval %s",String(target->getNewVal()));
WEBLOG("Target GetVal %s",String(target->getVal()));
WEBLOG("CurrentGetVal %s",String(current->getVal()));
if (target->getNewVal() == 100){
    do_transmit();}
else {
  dist = ((target->getNewVal()-current->getVal())/10) * seconds;
  do_transmit();
  WEBLOG("Transmitted RAISE %s %s \n", code, String(dist));
  }
};


// UPDATE METHOD
  boolean update(){
    
                                  // update() method
    if(target->getNewVal()>current->getVal()){ 
          LOG1("Raising Shade from update\n");
          raise(); 
    } else 
    if(target->getNewVal()<current->getVal()){      // if the target-position requested is less than the current-position, simply log a "raise" message  
          LOG1("Lowering Shade from update\n");                     // ** there is nothing more to do - HomeKit keeps track of the current-position so knows lowering is required
          lower();                  // ** there is nothing more to do - HomeKit keeps track of the current-position so knows lowering is required
    }
    return(true);                               // return true
  } // update

  void loop(){                                     // loop() method
if (dist > 0){
    delay(dist);
    code="11000011001100110011001100110011001100111100001100111100001100111100110000110011001100110011001111000011110000110011001100110011001111000011110000111100001100110011001100";
    do_transmit();
    WEBLOG("Transmitted STOP %s %s\n", code, String(dist));
    dist =0 ;
    }
  if(current->getVal()!=target->getVal() && target->timeVal()>1000){          // If the targetvalue does not match the current value and 16 seconds ( full shade drop) then evaluate for type of position change.
    WEBLOG("position change Current: (%S) Target (%S). \n",String(current->getVal()), String(target->getVal()) );//DEBUG Print String
    WEBLOG("Target Newval %s",String(target->getNewVal()));
  WEBLOG("Target GetVal %s",String(target->getVal()));
  WEBLOG("CurrentGetVal %s",String(current->getVal()));
      current->setVal(target->getVal()); 
      WEBLOG("FROM LOOP AFTER SETVAL Target Newval %s",String(target->getNewVal()));
      WEBLOG("Target GetVal %s",String(target->getVal()));
      WEBLOG("CurrentGetVal %s",String(current->getVal()));

    }//if change

  } // loop
  
};// struct
