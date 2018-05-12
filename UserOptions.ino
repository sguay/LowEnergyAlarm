/****************Program Functions                          *****************/
void setProgram() {
  programMode = PROGRAM;


#ifdef DEBUG
  Serial.println("Starting Programming User Options");
#endif

  setP1();
  if (userQuit != true || userTimeout != true) {
    chirp(1);

    setP2();
    if (userQuit != true || userTimeout != true) {
      chirp(1);

      setP3();
      if (userQuit != true || userTimeout != true) {
        chirp(1);

        EEPROM.put(FIRST_RUN_FLAG, FIRST_RUN_FALSE);
      }

    }

  }

  if (userTimeout != true) {
    setDisplay("--", RIGHT, YELLOW, BLINK);
    setDisplay("--", LEFT, YELLOW, BLINK);
    chirp(1);
    printSettings();
    programMode = RUN;
  }

  //reset flags for next run
  userTimeout = false;
  userQuit = false;
}

//P1 Set Units
void setP1() {
  int option;
  int tempInt;
  int newUnits;

  //Retrieve Stored Values or set to defaults if first run
  EEPROM.get(FIRST_RUN_FLAG, tempInt);
  if (tempInt == FIRST_RUN_TRUE) {
    option = KNOTS;
  } else {
    EEPROM.get(UNITS, option);
  }

  //Get New values from user
  newUnits = getProgramOption("P1", option, 1, 3);
  if (userTimeout != true) {
    units=newUnits;
    EEPROM.put(UNITS, units);
  }
}

//P2 Set Stall Speed
void setP2() {
  int option;
  float tempFloat;
  int tempInt;
  float newStallSpeed;

  //Retrieve Stored Values or set to defaults if first run
  EEPROM.get(FIRST_RUN_FLAG, tempInt);
  if (tempInt == FIRST_RUN_TRUE) {
    switch (units) {
      case KPH:
        option = (int)knotsToKph(DEFAULT_STALL_SPEED);
        break;
      case KNOTS:
        option = (int)DEFAULT_STALL_SPEED;
        break;
      case MPH:
        option = (int)knotsToMph(DEFAULT_STALL_SPEED);
        break;
    };
  } else {
    EEPROM.get(STALL_SPEED, tempFloat);
    switch (units) {
      case KPH:
        option = (int)tempFloat;
        break;
      case KNOTS:
        option = (int)kphToKnots(tempFloat);
        break;
      case MPH:
        option = (int)kphToMph(tempFloat);
        break;
    };
  }

  //Get New Values from User
  switch (units) {
    case KPH:
      newStallSpeed = (float)getProgramOption("P2", option, 10, 99);
      break;
    case KNOTS:
      newStallSpeed = (float)knotsToKph(getProgramOption("P2", option, 10, 99));
      break;
    case MPH:
      newStallSpeed = (float)mphToKph(getProgramOption("P2", option, 10, 99));
      break;
  }
  if (userTimeout != true) {
    stallSpeed = newStallSpeed;
    EEPROM.put(STALL_SPEED, stallSpeed);
  }
}

//P3 Set Circuit Height
void setP3() {
  int option;
  float tempFloat;
  int tempInt;
  float newCircuitHeight;

  //Retrieve Stored Values or set to defaults if first run
  EEPROM.get(FIRST_RUN_FLAG, tempInt);
  if (tempInt == FIRST_RUN_TRUE) {
    switch (units) {
      case KPH:
        option = (int)DEFAULT_CIRCUIT_HEIGHT / 100;
        break;
      case KNOTS:
        option = (int)metersToFeet(DEFAULT_CIRCUIT_HEIGHT) / 100;
        break;
      case MPH:
        option = (int)metersToFeet(DEFAULT_CIRCUIT_HEIGHT) / 100;
        break;
    };

  } else {
    EEPROM.get(CIRCUIT_HEIGHT, tempFloat);
    switch (units) {
      case KPH:
        option = (int)(tempFloat / 100);
        break;
      case KNOTS:
        option = (int)(metersToFeet(tempFloat) / 100);
        break;
      case MPH:
        option = (int)(metersToFeet(tempFloat) / 100);
        break;
    };
  }

  //Get new Values from User
  switch (units) {
    case KPH:
      newCircuitHeight = (float)(getProgramOption("P3", option, 1, 99) * 100);
      break;
    case KNOTS:
      newCircuitHeight = (float)(feetToMeters(getProgramOption("P3", option, 1, 99)) * 100);
      break;
    case MPH:
      newCircuitHeight = (float)(feetToMeters(getProgramOption("P3", option, 1, 99)) * 100);
      break;
  }

  if (userTimeout != true) {
    circuitHeight = newCircuitHeight;
    EEPROM.put(CIRCUIT_HEIGHT, circuitHeight);
  }
}

int getProgramOption(char text[2], int option, int minValue, int maxValue) {
  char buf [2];
  int lastOption = option;

  getEncoderValue();
  getEncoderValue();
  setDisplay(text, LEFT, RED, BLINK);
  if (option < 10) {
    sprintf (buf, "0%d", option);
  } else {
    sprintf (buf, "%d", option);
  }
  setDisplay(buf, RIGHT, RED, BLINK);
  lastUserInput = millis();

  while (true) {
    wdt_reset();
    option += getEncoderValue();

    //if knob has moved
    if (option != lastOption) {
      lastUserInput = millis();
    }

    if (option >= maxValue) {
      option = maxValue;
    }
    if (option <= minValue) {
      option = minValue;
    }
    if (option != lastOption) {

      if (option < 10) {
        sprintf (buf, "0%d", option);
      } else {
        sprintf (buf, "%d", option);
      }
      setDisplay(buf, RIGHT, RED, SOLID);
      lastOption = option;
    }
    if (getClick() == CLICKED) {
      setDisplay(buf, RIGHT, GREEN, SOLID);
      setDisplay(text, LEFT, GREEN, SOLID);
      delay(BLINK_RATE * 2);
      break;
    }
    //allow user to exit
    if (getClick() == DOUBLECLICKED) {
      userQuit = true;
      break;
    }

    //exit if no input recently
    if (millis() - lastUserInput > USER_INPUT_TIMEOUT) {
      userTimeout = true;
      break;
    }
  }

  return (option);
}

void printSettings() {
#ifdef DEBUG
  switch (units) {
    case KPH:
      Serial.printf("Units: KPH\t");
      Serial.printf("Stall Speed set to: %.0f kph\t", stallSpeed);
      Serial.printf("Circuit Height Set at: %.0f m\n", circuitHeight);
      Serial.printf("(Units: %d\t Stall Speed %0.f kph\t Circuit Height Set at: %.0f m)\n", units, stallSpeed, circuitHeight);
      break;
    case KNOTS:
      Serial.printf("Units: KNOTS\t");
      Serial.printf("Stall Speed set to: %.0f knots\t", kphToKnots(stallSpeed));
      Serial.printf("Circuit Height Set at: %.0f\n feet", metersToFeet(circuitHeight));
      Serial.printf("(Units: %d\t Stall Speed %0.f kph\t Circuit Height Set at: %.0f m)\n", units, stallSpeed, circuitHeight);
      break;
    case MPH:
      Serial.printf("Units: MPH\t");
      Serial.printf("Stall Speed set to: %.0f mph\t", kphToMph(stallSpeed));
      Serial.printf("Circuit Height Set at: %.0f feet\n", metersToFeet(circuitHeight));
      Serial.printf("(Units: %d\t Stall Speed %0.f kph\t Circuit Height Set at: %.0f m)\n", units, stallSpeed, circuitHeight);
      break;
  };
#endif
}



