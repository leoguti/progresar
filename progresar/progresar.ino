// Select your modem:
#define TINY_GSM_MODEM_SIM800
//#define TINY_GSM_MODEM_SIM900
//#define TINY_GSM_MODEM_A6
//#define TINY_GSM_MODEM_A7
//#define TINY_GSM_MODEM_M590

// uncomment line for debug
//#define _DEBUG_

// Can be installed from Library Manager or https://github.com/vshymanskyy/TinyGSM
#include <TinyGsmClient.h>
#include <ThingerTinyGSM.h>

// Emulate Serial1 on pins 10/11 if HW is not present (use interrupt pin in RX for better performance)
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(9, 2); // RX, TX
#endif

#define USERNAME "Ubilogica"
#define DEVICE_ID "progresar"
#define DEVICE_CREDENTIAL "BceAqr2o"

// use your own APN config
#define APN_NAME "internet.comcel.com.co"
#define APN_USER "comcel"
#define APN_PSWD "comcel"

// set your cad pin (optional)
#define CARD_PIN ""


//Variables para conductividad 

long pulseCount = 0;  //a pulse counter variable
unsigned long pulseTime,lastTime, duration, totalDuration;
int interruptPin = 1; //corresponds to D2
int samplingPeriod=3; // the number of seconds to measure 555 oscillations

//--------------------------

//thermistor stuff -----------------------
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    

ThingerTinyGSM thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL, Serial1);

void setup() {
  // uncomment line for debug
  //Serial.begin(115200);

  // Serial for AT commands (can be higher with HW Serial, or even lower in SW Serial)
  Serial1.begin(57600);

  // set APN (you can remove user and password from call if your apn does not require them)
  thing.setAPN(APN_NAME, APN_USER, APN_PSWD);

  // set PIN (optional)
  // thing.setPin(CARD_PIN);

  // resource input example (i.e, controlling a digitalPin);
  pinMode(13, OUTPUT);
  thing["led"] << digitalPin(13);

  thing["sensores"] >> [](pson& out){
        out["temperature"] = temperature();
        out["conductivity"] = conductivity();
  };

  // resource output example (i.e. reading a sensor value)
  //thing["millis"] >> outputValue(millis());

  //thing["conductivity"] >> outputValue(conductivity());

  //thing["temperature"] >> outputValue(temperature());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}





float temperature(){
    uint8_t i;
    float average;
    int samples[NUMSAMPLES];
    // take N samples in a row, with a slight delay
    for (i=0; i< NUMSAMPLES; i++) {
      samples[i] = analogRead(THERMISTORPIN);
      delay(10);
    }
    // average all the samples out
    average = 0;
    for (i=0; i< NUMSAMPLES; i++) {
      average += samples[i];
    }
    average /= NUMSAMPLES;
    
    // convert the value to resistance
    average = 1023 / average - 1;
    average = SERIESRESISTOR / average;
    
    float steinhart;
    steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                 // Invert
    steinhart -= 273.15;                         // convert to C
    return steinhart;    
}



float conductivity(){
  attachInterrupt(interruptPin,onPulse,RISING); //attach an interrupt counter to interrupt pin 1 (digital pin #3) -- the only other possible pin on the 328p is interrupt pin #0 (digital pin #2)
  pulseTime=micros(); // start the stopwatch
  delay(samplingPeriod*1000); //give ourselves samplingPeriod seconds to make this measurement, during which the "onPulse" function will count up all the pulses, and sum the total time they took as 'totalDuration' 
  detachInterrupt(interruptPin); //we've finished sampling, so detach the interrupt function -- don't count any more pulses
  float freqHertz;
  if (pulseCount>0) { //use this logic in case something went wrong
    double durationS=(totalDuration/double(pulseCount))/1000000.; //the total duration, in seconds, per pulse (note that totalDuration was in microseconds)
    freqHertz=1./durationS;
  }
  else {
    freqHertz=0.;
  }
  return freqHertz;
}


void onPulse()  //ver si esta en COnd
{
  pulseCount++;
  lastTime = pulseTime;
  pulseTime = micros();
  duration=pulseTime-lastTime;
  totalDuration+=duration;
}
