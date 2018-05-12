/****************Flight Logic Functions                     *****************/
void setFlightMode() {
  //LANDED TAKEOFF CRUISE LANDING_START LANDING_FINAL
  int currentHeight = baroAltitude - groundLevel;
  if (flightMode == LANDED && airSpeed < 20) {
    groundLevel = baroAltitude;
  }

  if ((flightMode == LANDED) && (currentHeight > 10) && (currentHeight < (circuitHeight)) && (airSpeed > stallSpeed)) {
    flightMode = TAKEOFF;
#ifdef DEBUG_FLIGHTMODE
    Serial.println("Flight Mode set to TAKEOFF");
#endif
  }

  if ((flightMode == TAKEOFF) && (currentHeight > (circuitHeight))) {
    flightMode = CRUISE;
#ifdef DEBUG_FLIGHTMODE
    Serial.println("Flight Mode set to CRUISE");
#endif
  }

  if ((flightMode == CRUISE) && (currentHeight < circuitHeight)) {
    flightMode = LANDING_START;
#ifdef DEBUG_FLIGHTMODE
    Serial.println("Flight Mode set to LANDING_START");
#endif
  }

  if ((flightMode == LANDING_START) && (currentHeight < 25) ) {
    flightMode = LANDING_FINAL;
#ifdef DEBUG_FLIGHTMODE
    Serial.println("Flight Mode set to LANDING_FINAL");
#endif
    alarmSilenced = false;
  }

  if (flightMode == LANDING_FINAL && (baroAltitude - groundLevel) < (25) && airSpeed < stallSpeed - 20) {
    flightMode = LANDED;
#ifdef DEBUG_FLIGHTMODE
    Serial.println("Flight Mode set to LANDED");
#endif
    windSpeedsSet = false;
    alarmSilenced = false;
  }

}

void setWindSpeeds() {

  programMode = PROGRAM;

#ifdef DEBUG
  Serial.println("Getting user input for flight speeds");
#endif

  chirp(1);
  setDisplay(" ", LEFT, RED, BLINK);
  setDisplay("  ", RIGHT, RED, BLINK);

  //Set Windspeed
  int temp = getWindInput(LEFT, windSpeed);

  if (userTimeout != true) {
    windSpeed = temp;
    chirp(1);

    //Set WindGusts
    temp = getWindInput(RIGHT, gustSpeed);
    if (userTimeout != true) {
      gustSpeed = temp;
      chirp(1);
      delay(1000);
      chirp(1);
      windSpeedsSet = true;
      setDisplay("--", RIGHT, GREEN, SOLID);
      setDisplay("--", LEFT, GREEN, SOLID);
      programMode = RUN;
    }
  }

  userTimeout = false;
}

int getWindInput(int side, int oldSpeed) {
  char buf [2];
  int option = oldSpeed;
  int lastOption = option;

  chirp(1);

  if (option < 10) {
    sprintf (buf, "0%d", option);
  } else {
    sprintf (buf, "%d", option);
  }
  setDisplay(buf, side, RED, BLINK);

  getEncoderValue();
  getEncoderValue(); //get rid of spurious signals
  lastUserInput = millis();
  while (true) {
    wdt_reset();
    shouldAlarm();
    option += getEncoderValue();

    if (option != lastOption) {
      lastUserInput = millis();
      if (option >= 99) {
        option = 99;
      }
      if (option <= 00) {
        option = 00;
      }
      if (option != lastOption) {
        if (option < 10) {
          sprintf (buf, "0%d", option);
        } else {
          sprintf (buf, "%d", option);
        }
        setDisplay(buf, side, RED, SOLID);
        lastOption = option;
      }
    }
    if (getClick() == CLICKED) {
      setDisplay(buf, side, GREEN, SOLID);
      delay(BLINK_RATE * 2);
      break;
    }

    if (millis() - lastUserInput > USER_INPUT_TIMEOUT) {
      userTimeout = true;
      break;
    }
  }
  return (option);
}

void shouldAlarm() {

  minimumAirSpeed = 1.5 * stallSpeed + 0.5 * windSpeed + gustSpeed;

  if (flightMode == LANDING_START && programMode == RUN && airSpeed < minimumAirSpeed) {
    alarmActive = true;
  } else
  {
    alarmActive = false;
  }

}

void checkIfLandingPrep() {
  char buf [2];
  int displaySpeed;
  if (flightMode == LANDING_START || flightMode == LANDING_FINAL) {

    if (windSpeedsSet == true) {
      displaySpeed = getAirSpeedCurrentUnits();
      if (displaySpeed < 10) {
        sprintf (buf, "0%d", displaySpeed);
      } else {
        sprintf (buf, "%d", displaySpeed);
      }
      if (displaySpeed > minimumAirSpeed * 1.1) {
        setDisplay(buf, RIGHT, GREEN, SOLID);
      } else
      {
        if (displaySpeed > minimumAirSpeed) {
          setDisplay(buf, RIGHT, YELLOW, BLINK);
        } else {
          setDisplay(buf, RIGHT, RED, BLINK);
        }
      }
    }
    else {
      setWindSpeeds();
    }

  }

  if (flightMode == TAKEOFF) {

  }
}

int getAirSpeedCurrentUnits() {
  switch (units) {
    case KPH:
      return ((int)airSpeed);
      break;
    case KNOTS:
      return ((int)kphToKnots(airSpeed));
      break;
    case MPH:
      return ((int)kphToMph(airSpeed));
      break;
  };
  return (0);
}

