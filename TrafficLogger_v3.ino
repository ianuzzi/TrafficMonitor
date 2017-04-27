// Data on RTC ---------------- SDA
// Clock on RTC --------------- SCL
// Data in Pressure ----------- A1
// Clock in Pressure ---------- A0
// Trig on Ultrasonic --------- 5
// Echo on Ultrasonic --------- 6
// 5V on Ultrasonic ----------- USB





// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h" // RTC library
#include "HX711.h"  // Pressure sensor library
#include <SPI.h>
#include <SD.h>



HX711 scale;
RTC_DS3231 rtc;
// Pins for Ultrasonic
const int TRIG_PIN = 5;
const int ECHO_PIN = 6;

const int setup_led = 13;
const int ready_led = 12;
const int event_led = 11;

const int distance_threshold = 12;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//const unsigned int MAX_DIST = 14186; // Anything over 96 in (14186 us pulse) is "out of range"
unsigned long t1;
unsigned long t2;
unsigned long last_us_event;
unsigned long time_since_last_us_event;
unsigned long pulse_width;
float distance_measured;
const int chipSelect = 4; // for data logger
String timestamp;
String output;
bool pressure_event = false;
bool ultrasound_event = false;
float ultrasound_baseline;
float press_reading;
File dataFile;



void setup() {  
  
  digitalWrite(setup_led, HIGH);

  Serial.begin(9600);

  pinMode(setup_led, OUTPUT); //red LED for setup/not ready/error
  pinMode(ready_led, OUTPUT); //green LED for working
  pinMode(event_led, OUTPUT); //yellow LED for event triggered
 
  // **** RTC SETUP *********************************************************************************


  if (! rtc.begin()) {
   while (1);
  }

  if (rtc.lostPower()) {
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2017, 3, 22, 18, 34, 0));
  }




  // **** END RTC SETUP *****************************************************************************

  // **** PRESSURE SENSOR SETUP *********************************************************************

  scale.begin(A1, A0);
  scale.set_scale();
  scale.set_scale(5000.f);
  scale.tare(10);
  // **** END PRESSURE SENSOR SETUP *****************************************************************
 

  // **** ULTRASONIC SETUP *********************************************************************

  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(10);  
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Wait for pulse on echo pin
  while ( digitalRead(ECHO_PIN) == 0 );

  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min
  t1 = micros();
  while ( digitalRead(ECHO_PIN) == 1);
  t2 = micros();
  pulse_width = t2 - t1;
  ultrasound_baseline = pulse_width / 180;

  // **** END ULTRASONIC SETUP *****************************************************************  

  // **** DATALOGGER SETUP *****************************************************************  



  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {

    // don't do anything more:
    return;
  }


  char filename[15];
  strcpy(filename, "DATA--00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }
  
  dataFile = SD.open(filename, FILE_WRITE);

  // **** END DATALOGGER SETUP *****************************************************************  
  digitalWrite(setup_led, LOW);
  digitalWrite(ready_led, HIGH);

} // END SETUP



void loop() {
  
  // **** CLOCK SECTION *****************************************************************************
  
  DateTime now = rtc.now();

  timestamp = String(now.year()) + "/"  + pad_left(now.month()) + "/" + pad_left(now.day());
  timestamp += " " + pad_left(now.hour()) + ":" + pad_left(now.minute()) + ":" + pad_left(now.second());
  
  // **** END CLOCK SECTION ************************************************************************



  // **** PRESSURE SENSOR SECTION ******************************************************************

    press_reading = scale.get_units();
    
    if (pressure_event){


      if (press_reading < 50) pressure_event = false;
    
  } else {

    if (press_reading > 50) {
      digitalWrite(event_led, HIGH);
      pressure_event = true;
      record_an_event("Pressure Event");
      digitalWrite(event_led, LOW);
      
    }
  }
  
  // **** END PRESSURE SENSOR SECTION **************************************************************

  

  // **** ULTRASONIC SECTION *********************************************************************

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Wait for pulse on echo pin
  while ( digitalRead(ECHO_PIN) == 0 );

  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min
  t1 = micros();
  while ( digitalRead(ECHO_PIN) == 1);
  t2 = micros();

  
  pulse_width = t2 - t1;

  // Calculate distance in centimeters and inches. The constants
  // are found in the datasheet, and calculated from the assumed speed 
  //of sound in air at sea level (~340 m/s).
  distance_measured = pulse_width / 180;
  //distance_measured = ultrasound_baseline - distance_measured;



  
  if (ultrasound_event){

    if (abs(distance_measured - ultrasound_baseline) < distance_threshold ) ultrasound_event = false; // 
    
  } else {

    if (abs(distance_measured - ultrasound_baseline) > distance_threshold) {

      digitalWrite(event_led, HIGH);
      ultrasound_event = true;
      record_an_event("Ultrasound Event");
      digitalWrite(event_led, LOW);
    }
  }

  last_us_event = t2;





  // **** END ULTRASONIC SECTION *****************************************************************
dataFile.flush();
  
} // END LOOP



String pad_left(int x){

    String result = "";
    if (x < 10) result += "0";
    result += String(x);
    return result;
}

void record_an_event(String event_type){

  String output_string = timestamp + "\t" + event_type;

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(output_string);

  }
  // if the file isn't open, pop up an error:

  
}

