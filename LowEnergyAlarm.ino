/*Airspeed Alarm
  Scott R. Guay
  S_R_Guay@yahoo.com

*/

#define DEBUG 1
//#define DEBUG_SERIAL
//#define DEBUG_FLIGHTMODE

//Libraries
#include <LedControl.h> // LedControl - Version: Latest
#include <ClickEncoder.h> //https://github.com/0xPIT/encoder/blob/arduino/examples/ClickEncoderTest/ClickEncoderTest.ino
#include <TimerOne.h>
#include <TimerThree.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <CircularBuffer.h>

/****************Configuration Settings- Hardware           ****************/
#define BAUD_RATE 9600  //NMEA Standard is 4800
#define MOTOR_PIN 23
#define SPEAKER_PIN 20

//#define ENC_DECODER (1 << 2)
#define ROTARY_ENCODER1_A 30
#define ROTARY_ENCODER1_B 31
#define ROTARY_ENCODER1_SW 32

#define ROTARY_ENCODER2_A 0
#define ROTARY_ENCODER2_B 1
#define ROTARY_ENCODER2_SW 2

#define LED_DATA 35
#define LED_CLOCK 36
#define LED_LOAD 37

#define HEARTBEAT_LED 13

/****************Configuration Settings- Software           ****************/
//EEPROM Locations
#define FIRST_RUN_FLAG 0
#define UNITS 10
#define STALL_SPEED 20
#define CIRCUIT_HEIGHT 30

//Constants
#define RED 0
#define YELLOW 1
#define GREEN 2
#define CYAN 3
#define BLUE 4
#define MAGENTA 5
#define WHITE 6
#define LEFT 0
#define RIGHT 1
#define SOLID 0
#define BLINK 1
#define BLINK_RATE 500
#define BLANK_DELAY 30000
#define FIRST_RUN_TRUE 0x01
#define FIRST_RUN_FALSE 0x00
#define DEFAULT_STALL_SPEED 56
#define DEFAULT_CIRCUIT_HEIGHT 305

#define HELD 1
#define CLICKED 2
#define DOUBLECLICKED 3

#define DATA_VALID_PERIOD 10000

/****************Global Variables and related Constants     ****************/
//Program Mode
#define STARTUP 0
#define RUN 1
#define PROGRAM 2
#define PAUSED 3
#define ERROR 4
int programMode = STARTUP;

//Flight Mode
#define LANDED 0
#define TAKEOFF 1
#define CRUISE 2
#define LANDING_START 3 //when landing speed alarm is active
#define LANDING_FINAL 4 //when landing speed alarm is surpressed
int flightMode = LANDED;

//User Inputs
long windSpeed = 0; //kph
boolean windSpeedsSet = false; //has user engered a windspeed
long gustSpeed = 0; //kph
boolean alarmActive = false;
boolean alarmSilenced = false;
boolean sounderOn = false;

#define USER_INPUT_TIMEOUT 30000
long lastUserInput =0;
boolean userTimeout = false;
boolean userQuit = false;


//Vario Inputs
float airSpeed = 0; //kph
float baroAltitude = 0; //meters
float groundLevel = 0; //meters
boolean alarmStatus = 0; // 0=NoAlarm
boolean validAirspeed = false;
long lastValidData;

//User Settings
#define KPH 1
#define KNOTS 2
#define MPH 3
int units;

//Run time Variables
long startTime = 0; //record when the system starts
float circuitHeight = 0;
float stallSpeed = 0;
long minimumAirSpeed = 0;
long previousUpdate = 0;

//Serial Data Variables
const byte numChars = 82;
char receivedChars[numChars];
char tempChars[numChars];
boolean newData = false;

//Rotary Encoder Variables
int16_t last, value;

//Display Buffer
char displayCharacters[4] = {0, 0, 0, 0};
char displayColours[4] = {0, 0, 0, 0};
char displayStyles[4] = {0, 0, 0, 0};
byte displayChanged = 0;
char text[2] = {0, 0};
long lastDisplayUpdate = 0;
unsigned long previousMillis = 0;
boolean lastBlink = false;
boolean lastState = false;

//beeps
#define NO_BEEP 0
#define SHORT_BEEP 1
#define SHORT_BEEP_DURATION 100
#define NORMAL_BEEP 2
#define NORMAL_BEEP_DURATION 500
#define ERROR_BEEP 3
#define ERROR_BEEP_DURATION 2000
#define CIRCULAR_BUFFER_INT_SAFE
CircularBuffer<int, 10> beepBuffer;
int currentBeep =0;
long currentBeepStart =0;
long currentBeepEnd =0;

/****************Hardware Definitions                       *****************/
/*Pin Definitions - MAX7221*/
// inputs: DIN pin, CLK pin, LOAD pin. number of chips
LedControl mydisplay = LedControl(LED_DATA, LED_CLOCK, LED_LOAD, 1);

/*Pin Definitions - Rotary Encoder*/
//SW1 A, B, Button
ClickEncoder *encoder = new ClickEncoder(ROTARY_ENCODER1_A, ROTARY_ENCODER1_B, ROTARY_ENCODER1_SW, 4, false);



/****************Setup Function                             *****************/

void setup()
{
  startTime = millis();

  //https://circuits4you.com/2018/01/24/tutorial-on-arduino-watchdog-timer-setup/
  wdt_enable(WDTO_8S);

  //Start Serial Connection
  Serial.begin(BAUD_RATE);     //Opens serial port
  Serial5.begin(4800);

  /*Pin Definitions - Buzzer*/
  pinMode(SPEAKER_PIN, OUTPUT);

  /*Pin Definitions - Haptic Motor Driver*/
  pinMode(MOTOR_PIN, OUTPUT);

  /*Pin Definitions for Hearbeat LED */
  pinMode(HEARTBEAT_LED, OUTPUT);
  digitalWrite(HEARTBEAT_LED, HIGH);
  delay(1000);
  digitalWrite(HEARTBEAT_LED, LOW);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, HIGH);
  delay(1000);
  digitalWrite(MOTOR_PIN, LOW);

  //attach timer for rotary encoder
  last = -1;
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  //Setup LEDs
  setDisplay();

#ifdef DEBUG_LEDS
  for (int n = 0; n <= 7; n++) {
    for (int c = 0; c <= 9; c++) {
      mydisplay.setChar(0, n, c, false);
      delay(1000);
    }
    mydisplay.setChar(0, n, ' ', false);
  }
#endif

  Timer3.initialize(100 * 1000);
  Timer3.attachInterrupt(updateDisplay);
  //displayTest();
  int firstRun;
  EEPROM.get(FIRST_RUN_FLAG, firstRun);
  if (firstRun == FIRST_RUN_TRUE) {

#ifdef DEBUG
    Serial.println("Entering Setup Menu");
#endif

    setProgram();
  } else {

#ifdef DEBUG
    Serial.println("Reading setup values from EEPROM");
#endif

    EEPROM.get(UNITS, units);
    EEPROM.get(STALL_SPEED, stallSpeed);
    EEPROM.get(CIRCUIT_HEIGHT, circuitHeight);
    printSettings();
  }

  //Get initial Data from Vario
  while (programMode == 0) {
    readNMEAData();

    if (newData == true) {
      strcpy(tempChars, receivedChars);
      // this temporary copy is necessary to protect the original data because strtok() replaces the commas with \0
      parseNMEAData();
      newData = false;
    }

    if (baroAltitude > 0) {
      programMode = 1;
#ifdef DEBUG
      Serial.println("Valid data received from Vario");
      Serial.printf("BaroAltitude of %f used as ground level \n", baroAltitude);
#endif
      break;
    }
    //break if no valid data in 10 seconds
    if (startTime - millis() > 10000) {
      programMode = 4;
#ifdef DEBUG
      Serial.println("No valid data received from Vario");
      baroAltitude = 0;
      Serial.printf("BaroAltitude of %f used as ground level \n", baroAltitude);
#endif
      break;
    }
  }

  //set ground level reference
  groundLevel = baroAltitude;
  setDisplay("--", RIGHT, GREEN, SOLID);
  setDisplay("--", LEFT, GREEN, SOLID);
  chirp(3);
  programMode = RUN;
}

/****************Main Loop                                  *****************/
void loop()
{
  wdt_reset();
  readNMEAData();

  if (newData == true) {
    strcpy(tempChars, receivedChars);
    // this temporary copy is necessary to protect the original data
    //   because strtok() replaces the commas with \0
    parseNMEAData();
    newData = false;
  }
  checkIfDataValid();
  setFlightMode();
  shouldAlarm();
  checkIfLandingPrep();
  doHMI();

#ifdef DEBUG
  debugInfo();
#endif
}

void debugInfo() {
  long currentMillis = millis();
  if (currentMillis - previousUpdate > 5000) {
    previousUpdate = currentMillis;
    Serial.printf("%d\t", millis());
    switch (flightMode) {
      case LANDED:
        Serial.printf("Mode: LANDED\t");
        break;
      case TAKEOFF:
        Serial.printf("Mode: TAKEOFF\t");
        break;
      case CRUISE:
        Serial.printf("Mode: CRUISE\t");;
        break;
      case LANDING_START:
        Serial.printf("Mode: LANDING_S\t");;
        break;
      case LANDING_FINAL:
        Serial.printf("Mode: LANDING_F\t");;
        break;
    };

    Serial.printf("Alarm: %s\t", alarmActive ? "true" : "false");
    Serial.printf("Altitude(AGL): %.1f\t Airspeed: %.1f\t Altitude (ASL): %.0f\t GroundLevel (ASL): %.0f\t CircuitHeight: %.0f\n", metersToFeet(baroAltitude - groundLevel), kphToKnots(airSpeed), metersToFeet(baroAltitude), metersToFeet(groundLevel), metersToFeet(circuitHeight));

  }
}

void checkIfDataValid() {

  long currentMillis = millis();
  if (currentMillis - lastValidData > DATA_VALID_PERIOD) {
    validAirspeed = false;
    setDisplay("Er", LEFT, RED, BLINK);
    setDisplay("01", RIGHT, RED, BLINK);
#ifdef DEBUG
    //Serial.println("No Valid Airspeed data received");
#endif
  } else
  {
    if (validAirspeed == false) {
      setDisplay("  ", LEFT, RED, SOLID);
      setDisplay("  ", RIGHT, RED, SOLID);
      validAirspeed = true;
    }
  }
}

void doHMI() {
  int clickValue = getClick();
  if (clickValue == CLICKED) {
#ifdef DEBUG
    Serial.println("Main Loop Click");
#endif
    if (alarmActive == true) {
      alarmSilenced = true;
    } else {
      setWindSpeeds();
    }
  }
  if (clickValue == DOUBLECLICKED) {
#ifdef DEBUG
    Serial.println("Main Loop Double Click");
    setProgram();
#endif
  }

  if (clickValue == HELD) {
#ifdef DEBUG
    Serial.println("Main Loop Held");
    EEPROM.put(FIRST_RUN_FLAG, FIRST_RUN_TRUE);
    while (true);
#endif
  }
}

