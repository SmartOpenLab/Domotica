/*

////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////    SMART SUN    //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

by Antonio Gordillo.
in Caceres. Spain. 2014.
Creative Common License BY-NC-SH


Based on: 
- AC Light Control  Updated by Robert Twomey 
 Zero-crossing detection to dim the light
 Adapted from sketch by Ryan McLaughlin 
 http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1230333861/30
 
- Tiny RTC I2C Module to control the real time 
 

V11.0: Includes:
  - Adaptation to the Arduino Mini Pro board
  - Turn the light off when a given threshold is readed in a analog input pin
  - Switch for on/off the light sensor (LDR) functionality
  - Switch for on/off the timer functionality
  - Read the sunrise and sunset time from potentiometers

// To include....
   //- Switch for set the clock time to 12:00h after pressing 3 seconds 
   //- Implement the dim control using the actual hour and using the lectures for sunrise and sunset
  
*/


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Libraries invocations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include  <TimerOne.h>          // Avaiable from http://www.arduino.cc/playground/Code/Timer1
#include <Wire.h>  // Incluye la librería Wire
#include "RTClib.h" // Incluye la librería RTClib

#define SERIAL_ON // used to send dat through serial or not

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pin definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A5 and A4 are for the RTC (SDL and SCA, respectively)
// Pin 2 is used to detect the interruption of the raising edge of the 220VAC signal

int AC_pin = 4;               // Output to Opto Triac
int LDRinputPin  = A0  ;      // Input from the LDR
int ThresholdPin = A3  ;      // Input from the LDR

int sunriseTimePin = A1  ;     // Input from the set-sunrise-time potentiometer
int sunsetTimePin  = A2  ;     // Input from the set-sunset-time potentiometer

int LDRswitchPin    = 6 ;      // Pin for the digital input to enable the light sensor
int timerswitchPin  = 7;       // Pin for the digital input to enable the timer to control the light level
int setClockTimePin = 8;       // Pin for the digital input to set the clock time to 12:00h.
int setClockLedPin  = 9;       // Pin for the digital output to blink the led after setting time.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Variable definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

volatile int i=0;               // Variable to use as a counter
volatile boolean zero_cross=0;  // Boolean to store a "switch" to tell us if we have crossed zero

int dim = 0;                    // Dimming level (0-128)  0 = on, 128 = 0ff

int freqStep = 167;    // This is the delay-per-brightness step in microseconds.
// It is calculated based on the frequency of the voltage supply (50Hz)
// and the number of brightness steps. 
// 
// The chopper circuit chops the AC wave twice per cycle, once on the positive half and once
// at the negative half. This meeans chopping happens at 120Hz for a 60Hz supply or 100Hz for a 50Hz supply. 

// To calculate freqStep divide the length of one full half-wave of the power
// cycle (in microseconds) by the number of brightness steps. 
//
// (1000000 uS / 100 Hz) / 60 brightness steps = 167 uS / brightness step
//
// 1000000 us / 100 Hz = 10000 uS, length of one half-wave.


//LDR sensor variables
int LDRswitchValue;
int LDRinputValue;
int threshold;

//timer variables
RTC_DS1307 RTC; // RTC object definition
int timerswitchValue;
int sunriseTime;
int sunsetTime;
int seconds;
int minutes;
int hours;
int minutes_from_midnight;


// millis variables
long millis_waiting_time = 2000; // interval to change the dimming level
unsigned long currentMillis;
unsigned long previousMillis;


/////////////// Functions  ////////////////////////

// set flag when interrupt is detected (rising edge) and turn off the triac
void zero_cross_detect() {    
  zero_cross = true;               // set the boolean to true to tell our dimming function that a zero cross has occured
  i=0;
  digitalWrite(AC_pin, LOW);       // turn off TRIAC (and AC)
}                                 

// Turn on the TRIAC at the appropriate time indicated by dim
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


// Light level control using the date. Test version with a complete day of only 4 minutes.
void timer_light_control_4minutes() {
          ////////////////////////////////  
         // Daytime Light level control
         ////////////////////////////////
           ///// Playing with a 4 minutes day
         // starting the sunrise at minute 0
         if(((minutes%4)==0)){
           #ifdef SERIAL_ON
             Serial.print("  Current Mode: SUNRISE ");
             Serial.println();
           #endif
           dim=(59-seconds);
           }
           //keeping the maximum light in the minute 1
         if(((minutes%4)==1)){
           #ifdef SERIAL_ON
             Serial.print("  Current Mode: DAY ");
             Serial.println();
           #endif
           dim=0;
           }
         // starting the sunset at minute 2
         if(((minutes%4)==2)){
           #ifdef SERIAL_ON
             Serial.print("  Current Mode: SUNSET ");
             Serial.println();
           #endif
           dim=seconds;
         }
         // keeping the minimum light during minute 3
         if(((minutes%4)==3)){
           #ifdef SERIAL_ON
             Serial.print("  Current Mode: NIGHT ");
             Serial.println();
           #endif
           dim=59;
         }    
}


// Print information in Serial port
void print_to_serial() {

    Serial.println();
    if(LDRswitchValue==HIGH){
      Serial.print("LDR control enabled");
      Serial.println();

      //send data to serial port
      Serial.print("  LDR input value: ");
      Serial.print(LDRinputValue);
      Serial.println();
      Serial.print("  Threshold value: ");
      Serial.print(threshold);
      Serial.println();
    }
    else
    { 
      Serial.print("LDR control disabled");
      Serial.println();
    }  

   if(timerswitchValue==HIGH){
      Serial.print("Timer control enabled");
      Serial.println();
      Serial.print("  Time: ");
      Serial.print(hours, DEC);
      Serial.print(':');
      Serial.print(minutes, DEC);
      Serial.print(':');
      Serial.print(seconds, DEC);
      Serial.println();
      Serial.print("  Minutes from midnight:  ");
      Serial.print(minutes_from_midnight);
      Serial.println();
      Serial.print("  Sunrise time: ");
      Serial.print(sunriseTime/60);
      Serial.print(':');
      Serial.print(sunriseTime%60);
      Serial.print("h   ;   Sunset time: ");
      Serial.print(sunsetTime/60);
      Serial.print(':');
      Serial.print(sunsetTime%60);
      Serial.print("h");
      Serial.println();
   


   }
   else
   {  
      Serial.print("Timer control disabled");
      Serial.println();
  }   
 
}

///////////////////// SETUP /////////////////////////////////

void setup() {                                      // Begin setup

// timer setup (including interruptions)
  // Use the TimerOne Library to attach an interrupt
  // to the function we use to check to see if it is 
  // the right time to fire the triac.  This function 
  // will now run every freqStep in microseconds.                                            

  #ifdef SERIAL_ON
   Serial.begin(9600); // Set data speed in the serial port
  #endif

  Wire.begin(); // Set the wire communication
  RTC.begin(); // Set the RTC functionality

//  RTC.adjust(DateTime(__DATE__, __TIME__)); // Set the date according to the connected PC's time. Uncomment to set the clock after battery replacement.

  pinMode(AC_pin, OUTPUT);                          // Set the Triac pin as output
  attachInterrupt(0, zero_cross_detect, RISING);   // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check, freqStep);      

//Digital I/O pin modes
  pinMode(AC_pin, OUTPUT);               // Pin for turning ON/OFF the Triac.
  pinMode(LDRswitchPin, INPUT);          // Pin to enable the light sensor
  pinMode(timerswitchPin, INPUT);        // Pin to enable the timer to control the light level
  pinMode(setClockTimePin, INPUT);       // Pin to set the clock time to 12:00h.
  pinMode(setClockLedPin, OUTPUT);       // Pin to blink the led after setting time.


}

///////////////////// LOOP /////////////////////////////////

void loop() {                        

  currentMillis = millis(); // Get current mseconds from millis

  DateTime now = RTC.now(); // Get current date from RTC


  // read the value from the LDR sensor ON/OFF switch:
  LDRswitchValue = digitalRead(LDRswitchPin);
  // read the value from the timer ON/OFF switch
  timerswitchValue = digitalRead(timerswitchPin);
  // read the value from the sensor:
  LDRinputValue = analogRead(LDRinputPin);
  // read the value from the threshold potentiometer
  threshold = analogRead(ThresholdPin);

  // read the value from the sunrise time potentiometer
  sunriseTime = analogRead(sunriseTimePin);
  sunriseTime = map(sunriseTime, 0, 1023, 0, 195);  // read the value from the sunset time potentiometer
  //converting in minutes from 00:00h taking into account that the limits are 4:53h and 8:07h
  sunriseTime += 293;  // 293 minutes from midnight till 4:53h

  sunsetTime = analogRead(sunsetTimePin);
  sunsetTime = map(sunsetTime, 0, 1023, 0, 195);  // read the value from the sunset time potentiometer
  //converting in minutes from 00:00h taking into account that the limits are 4:53h and 8:07h
  sunsetTime += 1133;  // 293 minutes from midnight till 18:53h


   // auxiliar variables
  minutes_from_midnight=minutes+hours*60;
  seconds=now.second();
  minutes=now.minute();
  hours=now.hour();


  // Temporized stuff using millis()
  if(currentMillis - previousMillis > millis_waiting_time){
     // guardar el ultimo instante en el que se movio el servo 
     previousMillis = currentMillis;   


  // Printing to serial port  
    #ifdef SERIAL_ON
      print_to_serial();
    #endif
 
  ////////////////////////////////  
  // LDR control
  ////////////////////////////////
    // when the sensor functionality is ON   
    if(LDRswitchValue==HIGH){

      // Turn the light off when thresold is reached 
      if(LDRinputValue>=threshold){
        dim=59; // turn off light       
      }   
      else {// else if threshold is not reached, use the timer control or turn it on

        // If timer is enabled
        if(timerswitchValue==HIGH){
          timer_light_control_4minutes();  
        } // end if the timer switch is ON
        else{ // end if the timer switch is ON turn ON the light
          dim=0;
        } 
 
     } // end else if not threshold is reached
 
   }  // end when the sensor functionality is ON   
   else
   {  // else when the sensor functionality is OFF   

       // If timer is enabled
       if(timerswitchValue==HIGH){
         timer_light_control_4minutes();  
      } // end if the timer switch is ON
      else
      { // end if the timer switch is OFF and the LDR sensor is OFF turn ON the light
         dim=0;
   } 

  }  // end when the sensor functionality is OFF

 } // end of the millis() condition
  
//  delay(1000);

}  /////////////////////// END LOOP ///////////////////////
