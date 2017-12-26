/*
  A  1...2' planters left
  A  2...2' planters right
  A  3...3' planters
  A  4...60 wall garden
  A  5...20 wall garden + 8 pots
  B  9...planter
  B  10..green house
  B  11...pots
  B  12...Rain Dance (Put R as 12th digit in type_of_zone3)
*/

/*
 * Type of Zones:
 *    N: Normal
 *    R: Rain Dance
 *    A: Alternate days
 *    B: Once a day
 */


const int number_of_pipes            = 12; // the delay required for an audible click
const int how_many_times_in_a_day    = 2; // the delay required for an audible click
const int start_time[] = {                 // at the clock hour that the flow should start range is 0..23
  7, 17,
};

const int flow_time[] = {                   // in seconds remember, one hour has 3600 seconds!
  10, 10, 15, 30, 30, 0, 0, 0,    // Board # A
  90, 90, 90, 180, 0, 0, 0, 0,    // Board # B
  0
};


//-----------------------------------------------------------------------------------------------------------------
//                                                                E  N  D     O  F    S  I  M  P  L  E      P  A  R  A  M  A  T  E  R  S
//                                                      0000000001111111111222222222233333333334444444444555555555566666
//                                                      1234567890123456789012345678901234567890123456789012345678901234
String  type_of_zone                      = String    ("NNNAANNNNNBRNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN");           // As defined on TOP. 

//                                                                E  N  D     O  F    C  O  M  P  L  E  X      P  A  R  A  M  A  T  E  R  S
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------
//TECH Maali has the following feedback sounds for client observation:

// While it is in "WAIT" or "DO NOTHING" mode, it emits a click sound once per second. RTC is accessed to emit this sound. No click implies RTC is not OK
// While it is in WATERING mode, it emits a louder click sound once per second. This click does not depend on RTC. So that, Device will function in Forced Switch mode.

// This version of TECH Maali has the following enhancements incorporated :
// Debounce, RTC, Watering cycle explicitlycoincides with 0 min and 0 seconds, Beeps for Init sequence,

/*

  for the TECH Maali, if green pin is used for directly connecting to the jumper location, it means only one decoder (138) is connected
  toggle switch connects to digital pin 10,
  11..13 motor +, speaker, LED respectively
  circuit modifications : short U1 pins 4,5,8 ; 6,16


  for the TECH Maali, pins 2..7 used for addressing the 138 important ... implement circuit modifications
  brown                    2
  voilet                   3
  grey                     4
  green                    5
  blue       force switch  10
  white      motor         11
  yellow     speaker       12
  LED                      13

  circuit modifications : short U1 pins 4,5,8 ; 6,16

  ethernet pins color codes :
  WHITE ORANGE            1
  ORANGE                  2
  WHITE GREEN             4
  BLUE                    3
  WHITE BLUE              RETURN
  GREEN                   5
  WHITE BROWN             PUMP
  BROWN                   6
*/

boolean motor_on = LOW;                  // MOTOR will come on when the signal is high / low on the motor pin.... needs to be HIGH in 40A relay box, driven by driver board
// On using SSR other than TECH Maali Board, motor_on parameter has to be HIGH.
// On using SSR on TECH Maali Board, motor_on parameter has to be  LOW because we use the resistor!li
boolean light_on = LOW;                  // Light will come on when the signal is high / low on the motor pin

boolean detect_power = LOW;            // power detection circuit will give a low signal when power exists
boolean detect_water = HIGH;            // power detection circuit will give a low signal when power exists





//--------------------------------------------------------------------------------------------------------------------------------------------
//                                                       Define the libraries that are to be included here
#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>                  // for RTC function
#include <Bounce2.h>                    // for debounce functioning of the FORCE_SWITCH


//--------------------------------------------------------------------------------------------------------------------------------------------
//                                                       Set the tresholds of all parameters monitored.
const int Rain_Dance_Treshold         = 950;
const int samples                     = 1000;
const int Power_Detect_Treshold       = 950;               // power is on @ around 855 - 876, and off @ around 1020
const int Water_Detect_Treshold       = 950;               // power is on @ around 855 - 876, and off @ around 1020
//const int Force_Switch_Treshold       = 950;
const int Soil_Moisture_Treshold      = 800;
const int Daylight_Treshold           = 800;
const int Humidity_Treshold           = 800;
const int pH_Treshold                 = 1800;

//--------------------------------------------------------------------------------------------------------------------------------------------
//                                                       Define all analog pins to be used.

#define RAIN_DANCE          A0               // Push button switch will trigger the rain dance cycle onto the last solenoid
//#define UT_BROWN            A1               // Upper Tank Brown wire .... connects to float ... Will read LOW when tank is full
#define LT_BLUE            A2               //  Water Tank Blue wire  .... connects to float ... Will read LOW when tank is empty
//#define FORCE_SWITCH       A2               // Upper Tank NEW wire .... connects to flow sensor ... Will read LOW when water is flowing
#define POWER_DETECT_PIN   A3               // for opto_coupler sensor plugging .... will effect scheduled watering only when Day
#define HUMIDITY           A4               // for opto_coupler sensor plugging .... will effect scheduled watering only when Day

//--------------------------------------------------------------------------------------------------------------------------------------------
//                                                       Define the Digital Pins .


int solenoid_connector_start_pin    =     2;             // brown, voilet, grey, green, this is to offset the startpin from the counter upto pin 7
#define Starter_ON                        8              // for large pump starter switch integrated into TECH Maali
#define Starter_OFF                       9              // for large pump starter switch integrated into TECH Maali
#define force_switch                     10              // for switch TECH Maqali
#define Motor                            11              // for those places where the pressure is low
#define speaker                          12              // for clicker
#define LED                              13              // our dear indicator
//--------------------------------------------------------------------------------------------------------------------------------------------
//                                                        initialize all the variables to be used.

Bounce debouncer = Bounce();                // Instantiate a Bounce object
const int clicker                 =     1;  // the delay required for an audible click
const int intra_packet = 600;                // delay between packets sent to the wireless device.... required to avoid buffer  over run!

boolean switch_state_was = LOW;             // previous switch state.... used to know if there was a change in switch position
boolean switch_state = LOW;                 // present switch state
boolean light_is_on = LOW;                  // for night light
boolean it_is_night_now = LOW;              // for night light
boolean debug = LOW; //HIGH;                // serial monitor messages

int temporary = 0;                          // stores present hour
int tempsec = 0;                            // stores present seconds
//int iterations = 0;                         // used only for diagnostics
int pauseBetweenRhythm = 100;              // another delay parameter
int justlikethati = 910;

//String  show_this_line = String (" ");           // a buffer string to build up full line displays
String  these = String ("    ");           // note : leading space is essential!



//--------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);
  while (!Serial) ;                                   // wait for serial
  delay(200);
  Serial.println();

  pinMode(RAIN_DANCE, INPUT);                             // initialize analog pins
  digitalWrite(RAIN_DANCE, HIGH);
  pinMode(LT_BLUE, INPUT);                             // initialize analog pins
  digitalWrite(LT_BLUE, HIGH);
  pinMode(POWER_DETECT_PIN, INPUT);
  digitalWrite(POWER_DETECT_PIN, HIGH);
  //pinMode(FORCE_SWITCH, INPUT);
  //digitalWrite(FORCE_SWITCH, HIGH);
  /*                      this code is replicated for the sake of understanding the way the PString works

    str.begin();
    str.print("The magic of str, starts here :The value of Secs is ");
    int current_millis_value = millis();
    str.print(current_millis_value);
    str.print(" I am amazed ! Iteration is ");
    str.print(iteration);

    my_print(str);
  */
  Serial.print("TECH Maali _v2 ..Force Switch on Digial Pin ");

  Serial.print(".. Rain Dance ");

  Serial.print(".. Suspend on Power Detect is ");

  if (detect_power == HIGH) {
    Serial.print("Enabled.... Present reading : ");
    boolean POWER_PRESENT = LOW;
    POWER_PRESENT = ReadSens_and_Condition(POWER_DETECT_PIN, Power_Detect_Treshold);
    if (POWER_PRESENT)  {
      Serial.println (" ON");
    }
    else
    {
      Serial.println (" OFF");
    }
  }
  else
  {
    Serial.println( "Disabled... will not suspend for Power outage");
  }

  Serial.print(".. Suspend on Water absence is ");

  if (detect_water == HIGH) {
    Serial.print("Enabled.... Present reading : ");
    boolean WATER_ABSENT = LOW;
    WATER_ABSENT = ReadSens_and_Condition(LT_BLUE, Water_Detect_Treshold);
    if (WATER_ABSENT)  {
      Serial.println (" Tank Empty");
    }
    else
    {
      Serial.println (" Tank Full");
    }
  }
  else
  {
    Serial.println( "Disabled... will not suspend for Water outage");
  }



  for (int thisrelay = solenoid_connector_start_pin; thisrelay <= LED; thisrelay++) {
    if (thisrelay != force_switch)    {
      pinMode(thisrelay, OUTPUT);
      digitalWrite (thisrelay, HIGH);
    }
  }
  digitalWrite(Starter_ON, HIGH);      // turn off
  digitalWrite(Starter_OFF, HIGH);      // turn off

  // setup force_switch to be addressed by the debounce library
  pinMode(force_switch, INPUT_PULLUP);    // Setup the button with an internal pull-up :
  // After setting up the button, setup the Bounce instance :
  debouncer.attach(force_switch);
  debouncer.interval(150); // interval in ms
  tmElements_t tm;
  if (RTC.read(tm))   temporary = tm.Hour;

  /*  Serial.print(" Hr      Probe : ");
    Serial.print(analogRead(probe));
  */
  Serial.print("  Zones : ");
  Serial.print(number_of_pipes);
  debouncer.update();
  switch_state = debouncer.read();
  switch_state_was = switch_state;
  Serial.print("  Switch : ");
  Serial.print(switch_state);

  Serial.print("  Start Times : ");

  for (int thisrelay = 0; thisrelay < how_many_times_in_a_day; thisrelay++) {
    Serial.print(start_time[thisrelay]);
    Serial.print(";  ");
  }

  Serial.print("\n  Zone Type : ");
  for (int thisrelay = 0; thisrelay < number_of_pipes; thisrelay++) {
    Serial.print(" # ");
    print2digits(thisrelay + 1);
    Serial.print(" : ");
    Serial.print(type_of_zone.charAt(thisrelay));                         // ... determine the type of garden for this relay
    Serial.print(",  ");
    if (((thisrelay + 1) % 8) == 0)  {
      Serial.println("  ");
      Serial.print("              ");
    }
  }
  Serial.println("");
  Initialize_device();
  //digitalWrite (lights_pin, !light_on);                                     //turn off light controller
  displaytime();
  hum_taiyaar_hai();
}                                                                          // end of setup


void  displaytime() {
  tmElements_t tm;
  if (RTC.read(tm))
    Serial.print("  Now the time reads : ");
  //      Serial.print("Ok, Time = ");
  //print2digits(temporary);
  print2digits(tm.Hour);
  Serial.write(':');
  print2digits(tm.Minute);
  Serial.write(':');
  print2digits(tm.Second);
  Serial.print(", Date (D/M/Y) = ");
  Serial.print(tm.Day);
  Serial.write('/');
  Serial.print(tm.Month);
  Serial.write('/');
  Serial.println(tmYearToCalendar(tm.Year));
}                                                // end of displaytime



void hum_taiyaar_hai() {
  tone(speaker, 1000, 300);
  delay (300);
  tone(speaker, 700, 600);
  delay (600);
  tone(speaker, 1500, 300);
  delay (300);
  noTone(speaker);

  Serial.println("\n\n\nR E A D Y");
}                                        // end of hum_taiyaar_hai


void loop() {
  tmElements_t tm;
  // Update the Bounce instance :
  these = " ";
  debouncer.update();
  switch_state = debouncer.read();
  //    Serial.println(switch_state);
  //switch_state = digitalRead(FORCE_SWITCH);
  if (switch_state_was != switch_state) {                                         /// switch toggled by the user.... so, water NOW!
    Serial.print("Forced .. Switch : ");
    Serial.print(switch_state);
    these = (" NAB");           // manual override activated... so, water everything

    Serial.print("  Chosen : ");
    Serial.print(these);
    Serial.print("**  Length : ");
    Serial.println(these.length());
    Water_the_garden(these);
    //    waterthegarden();
  }
  ;
  boolean rain_dance_switch_status = HIGH;
  boolean POWER_PRESENT = LOW;
  rain_dance_switch_status = ReadSens_and_Condition(RAIN_DANCE, Rain_Dance_Treshold);
  if (rain_dance_switch_status == HIGH) {                                         /// switch pressed by the user.... so, rain dance NOW!
    Serial.print("Forced .. RAIN_DANCE : ");
    Serial.print(switch_state);
    these = (" R");           // R for Rain Dance,  manual override activated... so, water everything

    Serial.print("  Chosen : ");
    Serial.print(these);
    Serial.print("**  Length : ");
    Serial.println(these.length());
    Water_the_garden(these);
    //    waterthegarden();
  }

  if (RTC.read(tm)) {
    if (tempsec != tm.Second ) {   // send a click every second to the speaker
      /*          Serial.print(iterations);//
        Serial.print(" ");
      */
      click_on_speaker ();
      tempsec = tm.Second;
      //  digitalWrite (LED, !digitalRead(LED));

      //      iterations = 0;
    }
    //    iterations = iterations + 1;

    if ((temporary != tm.Hour ) && (tm.Minute == 0) && (tm.Second == 0)) {                                   // enter only on the stroke of the hour
      temporary = tm.Hour;
      
      //   build the string of these gardens zones whose time has come to water
      for (int i = 0; i < how_many_times_in_a_day ; i++) {                        //normal garden screening :
        if (start_time[i] == tm.Hour ) {
          these = these + "N";           
        }
      }


      if (((tm.Day % 2) == 0)) {
        if (start_time[0] == tm.Hour ) {
          these = these + "A";
        }
      }

      if (start_time[0] == tm.Hour ) {
        these = these + "B";
      }

      Water_the_garden(these);
      
      hum_taiyaar_hai();
    }
  }
}                                                                      //  end of loop



void Initialize_device() {
  //PORTB = B00000101 ; // does a high on 8 & 10 for enabling the 74LS138
  // init all relays for low state so that there is no wasteage.
  // give a click on all relays, so that user is comfortable
  //  Serial.println("");//hoho

  //  Serial.println();
  Serial.print  ("  Durations : ");
  digitalWrite (Motor, motor_on);                                 // turn ON the motor
  digitalWrite (Starter_ON, HIGH);
  delay (intra_packet);
  digitalWrite (Starter_ON, LOW);

  for (int thisrelay = 0; thisrelay < number_of_pipes; thisrelay++) {
    init_click_on_speaker();
    Serial.print(" # ");
    print2digits(thisrelay + 1);
    Serial.print(" : ");
    Serial.print(flow_time[thisrelay]);
    Serial.print(",  ");
    if (((thisrelay + 1) % 8) == 0)  {
      Serial.println("  ");
      Serial.print("              ");
    }
    PORTD = (thisrelay << solenoid_connector_start_pin) ;                                       // ... turn ON relevant device
    delay(100); // this is required for estabilishing the count of the relay
    PORTD = 0777 ; // to turn off all the devices
    delay(200);
  }
 

  Serial.println("");
  Serial.println("  TECH Maali Initialised... ");
  digitalWrite (Starter_OFF, HIGH);
  delay (intra_packet);
  digitalWrite (Starter_OFF, LOW);

  digitalWrite (Motor, !motor_on);                                 // turn OFF the motor
  digitalWrite( LED, LOW );
}                                                         // end of Initialize_device


void  click_on_speaker() { // plays a metronomic rhythm on the speaker
  //  analogWrite(speaker, 255);  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255
  tone(speaker, 1200); // hf click on the speaker
  digitalWrite (LED, !digitalRead(LED));
  delay(clicker);               // inserted multiplier
  noTone(speaker);
  //  digitalWrite (LED, LOW);
}


void  init_click_on_speaker() { // plays a beep on the speaker during init cycle for audio of pipe count
  tone(speaker, 1200); // hf click on the speaker
  digitalWrite (LED, HIGH);
  delay(clicker * 100);
  noTone(speaker);
  digitalWrite (LED, LOW);
}


void Water_the_garden(String selected) {

  displaytime();

  int variations = (selected.length());                                       // ... inform user interface about the type of variations that will be watered now!
  Serial.print("  Variations = ");
  Serial.print(selected);
  Serial.print("  length = ");
  Serial.println(variations);

  digitalWrite (Starter_ON, HIGH);
  delay (intra_packet);
  digitalWrite (Starter_ON, LOW);


  for (int thisrelay = 0; thisrelay < number_of_pipes ; thisrelay++) {          // ... serious business... watering cycle starts
    ///
    int     flow_secs = 0;
    char this_garden_is = type_of_zone.charAt(thisrelay) ;                    // ... determine the type of garden for this relay
    Serial.println();
    Serial.print("  Garden of type = ");
    Serial.print(this_garden_is);
    Serial.print(" ... ");

    int should_flow = selected.indexOf(this_garden_is);                         // ... determine if water should flow in this relay
    if (should_flow > 0 ) { 
        flow_secs = flow_time[thisrelay];                     // ... set time of flowing for this relay.... non zero value
    }


    if (flow_secs > 0) {
      digitalWrite (Motor, motor_on);                                               // ... turn ON the Motor
    }
 

    print2digits(thisrelay + 1);                                              // for human interface....
    Serial.print(" Tube ... Secs :");
    Serial.print(flow_secs);
    Serial.print(" >> ");

    PORTD = (thisrelay << solenoid_connector_start_pin) ; // ... turn ON relevant device... shift left command used
    //    Serial.print(endtime);

    for (int time_left = flow_secs; time_left > 0; time_left--) {                     // ... Click Click until end time is reached
      tone(speaker, 5000); // hf click on the speaker
      delay(clicker * 20);
      noTone(speaker);
      delay(justlikethati);

      boolean POWER_PRESENT = LOW;
      POWER_PRESENT = ReadSens_and_Condition(POWER_DETECT_PIN, Power_Detect_Treshold);
      //Serial.println(sens);             // debug value

      while (detect_power && !POWER_PRESENT) {                            // suspend count for power fail condition
        PORTD = 0777 ;                                                    // to turn off all the devices otherwise, LED will remain on using up precious battery backup power
        digitalWrite (Motor, !motor_on);                                 // turn OFF the motor otherwise, LED will remain on using up precious battery backup power
      
        delay (200);
        POWER_PRESENT = ReadSens_and_Condition(POWER_DETECT_PIN, Power_Detect_Treshold);
        //  Serial.println(sens);             // debug value
        digitalWrite (LED, !digitalRead(LED));                              // toggle LED
      };                                    //wait for power to come on




      // check out if water is on
      //      boolean is_the_power_on = digitalRead (POWER_DETECT_PIN);
      boolean WATER_ABSENT = LOW;
      WATER_ABSENT = ReadSens_and_Condition(LT_BLUE, Water_Detect_Treshold);
      //Serial.println(sens);             // debug value

      while (detect_water && WATER_ABSENT) {                            // suspend count for power fail condition
        PORTD = 0777 ;                                                    // to turn off all the devices otherwise, LED will remain on using up precious battery backup power
        digitalWrite (Motor, !motor_on);                                 // turn OFF the motor otherwise, LED will remain on using up precious battery backup power
   
        for (int i = 0; i <= 1; i++) {                                        // double beep  ... indicates that lower tank is empty
          click();
          delay(200);                                                     //
        }
        delay (500);
        WATER_ABSENT = ReadSens_and_Condition(LT_BLUE, Water_Detect_Treshold);
        digitalWrite (LED, !digitalRead(LED));                              // toggle LED
        //  Serial.println(sens);             // debug value
      };                                    //wait for power to come on







      PORTD = (thisrelay << solenoid_connector_start_pin) ; // ... turn ON relevant device... shift left command used
      digitalWrite (Motor, motor_on);                                 // turn OFF the motor otherwise, LED will remain on using up precious battery backup power
      Serial.print(time_left);
      Serial.print("..");
      if (((time_left) % 25) == 0)  {
        Serial.println();
        Serial.print("                                                  ");
      }
      if (((time_left) % 4) == 0)          digitalWrite (LED, HIGH);                              // while watering, duty cycle of 1:3 is desirable
      else         digitalWrite (LED, LOW);                              // toggle LED

    };                                                    // ... this is the time the solenoid will remain ON
    PORTD = 0777 ; // to turn off all the devices otherwise, last relay will remain on till next cycle
    Serial.println(" Stopped ");                   // for feedback
    digitalWrite (Motor, !motor_on);                                 // turn OFF the motor
    //    digitalWrite (LED, LOW);
    // Update the Bounce instance :
    debouncer.update();
    switch_state_was = debouncer.read();

    delay(500);                                        // this delay is to avoid electrical spikes
  }
  digitalWrite (Starter_OFF, HIGH);
  delay (intra_packet);
  digitalWrite (Starter_OFF, LOW);

  // this code is inserted to drain the feeder pipe quickly, to reduce the carrying load on the pressure pipe
  Serial.print("Draining.... ");

  PORTD = 0777 ; // to turn off all the devices otherwise, last relay will remain on till next cycle

  Serial.println("Draining done. ");
  hum_taiyaar_hai;
}                                                                      // end of Water_the_garden


void print2digits(int number) {

  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}                                            // end of print2digits


boolean ReadSens_and_Condition(int THIS_PIN, int THIS_TRESHOLD) {
  int i;
  long sval = 0;
  boolean IS_PRESENT = LOW;
  for (i = 0; i < samples; i++) {
    sval = sval + analogRead(THIS_PIN);    // sensor on analog pin 0
  }

  sval = sval / samples;    // average
  /*    Serial.print (THIS_PIN);
    Serial.print (',');
    Serial.println (sval);*/
  // Serial.print (" ");
  //int mapped = map(sval, 800, 1100, 0, 500);
  //sval = sval *2;    // scale (0 - 1000)
  //sval = 1024 - sval;  // invert output
  //      Serial.print(sens);
  //  Serial.print(" / ");
  //Serial.print(power_detect_treshold);
  if (sval < THIS_TRESHOLD) {
    //    Serial.print(" Lower, so, Power Present");
    IS_PRESENT = HIGH;
  }
  else {
    //    Serial.print(" Higher, so, Power Absent");
    IS_PRESENT = LOW;
  }

  return IS_PRESENT;                            //mapped;
}//                                           end of ReadSens_and_Condition






void click() {
  digitalWrite(speaker, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(30);
  digitalWrite(speaker, LOW);   // turn the LED on (HIGH is the voltage level)
  delay(30);
}
