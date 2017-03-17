
/* Based on: 
- AC Light Control  Updated by Robert Twomey 
 
 Changed zero-crossing detection to look for RISING edge rather
 than falling.  (originally it was only chopping the negative half
 of the AC wave form). 
 
 Also changed the dim_check() to turn on the Triac, leaving it on 
 until the zero_cross_detect() turn's it off.
 
 Adapted from sketch by Ryan McLaughlin 
 http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1230333861/30
 
- Tiny RTC I2C Module to control the real time 
 
 */

/*

V4.0: Includes:
  - Dimmer with 60 levels of luminosity
  - One day night/day cycle simulation with sunset and sunrise (4 minutes day for test)
  - Turn the light off when a given threshold is readed in a analog input pin

// To include....
   //- Switch for on/off the light sensor (LDR) functionality
    //- Switch for on/off the timer functionality
*/


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Libraries invocations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include  <TimerOne.h>          // Avaiable from http://www.arduino.cc/playground/Code/Timer1
#include <Wire.h>  // Incluye la librería Wire
#include "RTClib.h" // Incluye la librería RTClib

RTC_DS1307 RTC; // Crea el objeto RTC

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pin definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A5 and A4 are for the RTC (SDL and SCA, respectively)
int AC_pin = 11;                // Output to Opto Triac
int analogInPin = A3  ;           // Input from the LDR

int LDRswitchPin = 10 ;           // Pin for the digital input to enable the light sensor
int timerSwitchPin = 9;           // Pin for the digital input to enable the timer to control the light level

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Variable definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

volatile int i=0;               // Variable to use as a counter
volatile boolean zero_cross=0;  // Boolean to store a "switch" to tell us if we have crossed zero

int dim = 0;                    // Dimming level (0-128)  0 = on, 128 = 0ff

int freqStep = 167;    // This is the delay-per-brightness step in microseconds.
// It is calculated based on the frequency of your voltage supply (50Hz)
// and the number of brightness steps you want. 
// 
// The only tricky part is that the chopper circuit chops the AC wave twice per
// cycle, once on the positive half and once at the negative half. This meeans
// the chopping happens at 120Hz for a 60Hz supply or 100Hz for a 50Hz supply. 

// To calculate freqStep you divide the length of one full half-wave of the power
// cycle (in microseconds) by the number of brightness steps. 
//
// (1000000 uS / 100 Hz) / 60 brightness steps = 167 uS / brightness step
//
// 1000000 us / 100 Hz = 10000 uS, length of one half-wave.


//LDR sensor variables
int analogInValue;
int threshold=650;

//timer variables
int seconds;
int minutes;

// for the millis
long dim_change_interval = 1000; // interval to change the dimming level
unsigned long currentMillis;
unsigned long previousMillis;



void setup() {                                      // Begin setup

// timer setup (including interruptions)
  // Use the TimerOne Library to attach an interrupt
  // to the function we use to check to see if it is 
  // the right time to fire the triac.  This function 
  // will now run every freqStep in microseconds.                                            
  Serial.begin(9600); // Set data speed in the serial port
  Wire.begin(); // Set the wire communication
  RTC.begin(); // Set the RTC functionality

//  RTC.adjust(DateTime(__DATE__, __TIME__)); // Set the date according to the connected PC's time. Uncomment to set the clock after battery replacement.

  pinMode(AC_pin, OUTPUT);                          // Set the Triac pin as output
  attachInterrupt(0, zero_cross_detect, RISING);   // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check, freqStep);      

}

void zero_cross_detect() {    
  zero_cross = true;               // set the boolean to true to tell our dimming function that a zero cross has occured
  i=0;
  digitalWrite(AC_pin, LOW);       // turn off TRIAC (and AC)
}                                 

// Turn on the TRIAC at the appropriate time
void dim_check() {                   
  if(zero_cross == true) {              
    if(i>=dim) {                     
      digitalWrite(AC_pin, HIGH); // turn on light       
      i=0;  // reset time step counter                         
      zero_cross = false; //reset zero cross detection
    } 
    else {
      i++; // increment time step counter                     
    }                                
  }                                  
}                                   

void loop() {                        

  currentMillis = millis();

  DateTime now = RTC.now(); // Obtiene la fecha y hora del RTC
  
  // Temporized stuff using millis()
    if(currentMillis - previousMillis > dim_change_interval){
     // guardar el ultimo instante en el que se movio el servo 
     previousMillis = currentMillis;   

    Serial.println();
 
    Serial.print("Time: ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    seconds=now.second();
    minutes=now.minute();


  ////////////////////////////////  
  // LDR control
  ////////////////////////////////
 
    // read the value from the sensor:
    analogInValue = analogRead(analogInPin);
    // directly dim acording to the analog Input value scaled to the correct number of dimming steps
//    dim = map(analogInValue, 0, 1023, 0, 60);  

    Serial.print("Analog input value: ");
    Serial.print(analogInValue);

//    Serial.print("  ;   Dimming Level: ");
//    Serial.print(dim);
//    Serial.println();


// Test to turn the light off when a given thresold is readed 
// in the LDR anlaog input pin
  if(analogInValue>=threshold){
    dim=59;
  }
  else {// if threshold is reached, turn the light off


// else if threshold is not reached, use the timer control or turn it on (as enabled by the timer on/off switch in the future)
  
  ////////////////////////////////  
  // Light level control
  ////////////////////////////////
  
  ///// Playing with a 4 minutes day
  // starting the sunrise at minute 0
    if(((minutes%4)==0)){
      Serial.print("   Mode: SUNRISE ");
      dim=(59-seconds);
    }
    //keeping the maximum light in the minute 1
    if(((minutes%4)==1)){
      Serial.print("   Mode: DAY ");
      dim=0;
    }
    // starting the sunset at minute 2
    if(((minutes%4)==2)){
      Serial.print("   Mode: SUNSET ");
      dim=seconds;
    }
    // keeping the minimum light during minute 3
    if(((minutes%4)==3)){
      Serial.print("   Mode: NIGHT ");
      dim=59;
    }    
    /*
    //decreasing brightness the even minutes
    if((minutes%2==0)&&(dim<=59))
      dim+=1;
    //increasing brightness the odd minutes
    if((minutes%2!=0)&&(dim>=0))
      dim-=1;
 
 */
 /* dim+=inc;
  if((dim>=9) || (dim<=0))
    inc*=-1;*/
  
    } // end of the else if not threshold is reached
 
  } // end of the millis() condition
  
//  delay(1000);
}
