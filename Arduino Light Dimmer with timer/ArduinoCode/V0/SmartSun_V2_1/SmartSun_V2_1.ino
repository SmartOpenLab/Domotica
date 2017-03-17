
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

#include  <TimerOne.h>          // Avaiable from http://www.arduino.cc/playground/Code/Timer1

#include <Wire.h>  // Incluye la librería Wire
#include "RTClib.h" // Incluye la librería RTClib

RTC_DS1307 RTC; // Crea el objeto RTC



volatile int i=0;               // Variable to use as a counter
volatile boolean zero_cross=0;  // Boolean to store a "switch" to tell us if we have crossed zero
int AC_pin = 11;                // Output to Opto Triac
int dim = 0;                    // Dimming level (0-128)  0 = on, 128 = 0ff
int inc=1;                      // counting up or down, 1=up, -1=down

int seconds;
int minutes;

int freqStep = 1000;    // This is the delay-per-brightness step in microseconds.
// It is calculated based on the frequency of your voltage supply (50Hz)
// and the number of brightness steps you want. 
// 
// The only tricky part is that the chopper circuit chops the AC wave twice per
// cycle, once on the positive half and once at the negative half. This meeans
// the chopping happens at 120Hz for a 60Hz supply or 100Hz for a 50Hz supply. 

// To calculate freqStep you divide the length of one full half-wave of the power
// cycle (in microseconds) by the number of brightness steps. 
//
// (1000000 uS / 100 Hz) / 10 brightness steps = 1000 uS / brightness step
//
// 1000000 us / 100 Hz = 10000 uS, length of one half-wave.

void setup() {                                      // Begin setup

  Serial.begin(9600); // Establece la velocidad de datos del puerto serie
  Wire.begin(); // Establece la velocidad de datos del bus I2C
  RTC.begin(); // Establece la velocidad de datos del RTC
  RTC.adjust(DateTime(__DATE__, __TIME__)); // Establece la fecha y hora

  pinMode(AC_pin, OUTPUT);                          // Set the Triac pin as output
  attachInterrupt(0, zero_cross_detect, RISING);   // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check, freqStep);      

  // Use the TimerOne Library to attach an interrupt
  // to the function we use to check to see if it is 
  // the right time to fire the triac.  This function 
  // will now run every freqStep in microseconds.                                            
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
 
  DateTime now = RTC.now(); // Obtiene la fecha y hora del RTC
  
  Serial.print("Time: ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  seconds=now.second();
  minutes=now.minute();
 
  //decreasing brightness the even minutes
  if((minutes%2==0)&&(dim<=9))
    dim+=inc;

  //increasing brightness the odd minutes
  if((minutes%2!=0)&&(dim>=0))
    dim-=inc;
 
 /* dim+=inc;
  if((dim>=9) || (dim<=0))
    inc*=-1;*/
    
  delay(1000);
}
