
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
 

V7.0: Includes:
  - Adaptation to the Arduino Mini Pro board
  - Turn the light off when a given threshold is readed in a analog input pin
  - Switch for on/off the light sensor (LDR) functionality
  - Switch for on/off the timer functionality

// To include....
   //- Switch for set the clock time to 12:00h after pressing 3 seconds 
  
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
int LDRswitchValue;
int LDRinputValue;
int threshold;

//timer variables
int timerswitchValue;
int sunriseTime;
int sunsetTime;

int seconds;
int minutes;

// for the millis
long millis_waiting_time = 1000; // interval to change the dimming level
unsigned long currentMillis;
unsigned long previousMillis;



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

///////////////////// SETUP /////////////////////////////////

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

   // auxiliar variables
    seconds=now.second();
    minutes=now.minute();


  // Temporized stuff using millis()
  if(currentMillis - previousMillis > millis_waiting_time){
     // guardar el ultimo instante en el que se movio el servo 
     previousMillis = currentMillis;   

/*    Serial.print("LDR Sensor control enabled? ");
    Serial.print(LDRswitchValue);
    Serial.println();
    Serial.print("timer control enabled? ");
    Serial.print(timerswitchValue);
    Serial.println();
*/
    // Printing to serial port date
    Serial.println();
    Serial.print("Time: ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

 

  ////////////////////////////////  
  // LDR control
  ////////////////////////////////

    // at first the light is fully ON
//    dim=0;

    // when the sensor functionality is ON   
    if(LDRswitchValue==HIGH){
      Serial.print("LDR control enabled");
      Serial.println();

//      dim=0;
      
      //send data to serial port
      Serial.print("LDR input value: ");
      Serial.print(LDRinputValue);
      Serial.println();
      Serial.print("Threshold value: ");
      Serial.print(threshold);
      Serial.println();

      // Turn the light off when thresold is reached 
      if(LDRinputValue>=threshold){
//        digitalWrite(AC_pin, LOW);
        dim=59; // turn off light       
//        Serial.print("Threshold reached. Turn OFF light,");
//        Serial.println();
      }   
      else {// else if threshold is not reached, use the timer control or turn it on

//        Serial.print("Threshold not reached. Use the timer if enabled");
//        Serial.println();
      
        // If timer is enabled
        if(timerswitchValue==HIGH){
          Serial.print("Timer control enabled: ");
  
         ////////////////////////////////  
         // Daytime Light level control
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
  
       } // end if the timer switch is ON
       else{ // end if the timer switch is ON turn ON the light
          dim=0;
       } 
 
     } // end else if not threshold is reached
 
 

   }  // end when the sensor functionality is ON   
   else{  // else when the sensor functionality is OFF   
      Serial.print("LDR control disabled");
      Serial.println();
//      dim=59;
//      digitalWrite(AC_pin, LOW); // turn OFF the light____ DO NOT WORK CAUSE IN BECAUSE INTERRUPTS

     //     else when the sensor functionality is ON we can also control the 
     //   light with the timer.
  

       // If timer is enabled
       if(timerswitchValue==HIGH){
         Serial.print("Timer control enabled");
 
         ////////////////////////////////  
         // Daytime Light level control
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
  
      } // end if the timer switch is ON
      else{ // end if the timer switch is OFF and the LDR sensor is OFF turn ON the light
         Serial.print(" Timer control disabled");
         Serial.println();
          dim=0;
      } 

  
//    dim=0;

    }  // end when the sensor functionality is OFF


    Serial.println();

  } // end of the millis() condition
  
//  delay(1000);

}  /////////////////////// END LOOP ///////////////////////
