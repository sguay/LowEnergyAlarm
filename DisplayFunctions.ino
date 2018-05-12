/****************Display Functions                          *****************/
//Function Setups LED Display
void setDisplay() {

  mydisplay.shutdown(0, false);  // turns on display
  mydisplay.setIntensity(0, 15); // 15 = brightest
}

void displayTest() {
  char buf [2];
  for (int n = 0; n <= 9; n++) {

    if ((n + (n * 10)) < 10) {
      sprintf (buf, "0%d", (n + (n * 10)));
    } else {
      sprintf (buf, "%d", (n + (n * 10)));
    }

    setDisplay(buf, RIGHT, RED, SOLID);
    setDisplay(buf, LEFT, RED, SOLID);
    delay(1000);
    setDisplay(buf, RIGHT, YELLOW, SOLID);
    setDisplay(buf, LEFT, YELLOW, SOLID);
    delay(1000);
    setDisplay(buf, RIGHT, GREEN, SOLID);
    setDisplay(buf, LEFT, GREEN, SOLID);
    delay(1000);
  }
}

//Function called by interup timer. This function handles blinking the display and blanking it
void updateDisplay() {
  boolean shouldShowResult = shouldShow();

  if (shouldBlank()) {
    mydisplay.shutdown(0, true);  // turns off display
  } else
  {
    mydisplay.shutdown(0, false);  // turns on display

    //process alarm
    if (alarmActive == true && alarmSilenced == false)digitalWrite(SPEAKER_PIN, HIGH);
    else  digitalWrite(SPEAKER_PIN, LOW);

    //process beeps
    int onPeriod;
    int offPeriod;
    switch (currentBeep)
    {
      case SHORT_BEEP:
        onPeriod = SHORT_BEEP_DURATION;
        offPeriod = SHORT_BEEP_DURATION;
        break;
      case NORMAL_BEEP:
        onPeriod = NORMAL_BEEP_DURATION;
        offPeriod = SHORT_BEEP_DURATION;
        break;

      case ERROR_BEEP:
        onPeriod = ERROR_BEEP_DURATION;
        offPeriod = SHORT_BEEP_DURATION;
        break;
    }

    int currentMillis = millis();
    if (currentBeep != 0) {

      if (currentMillis - currentBeepStart > onPeriod) digitalWrite(SPEAKER_PIN, LOW);
      if (currentMillis - currentBeepStart > onPeriod + offPeriod) currentBeep = 0;
    } else
    {
      if (!beepBuffer.isEmpty()) {
        digitalWrite(SPEAKER_PIN, HIGH);
        currentBeepStart = currentMillis;
        currentBeep = beepBuffer.pop();
      }
    }


    if (alarmActive == true && alarmSilenced == false) digitalWrite(MOTOR_PIN, HIGH);
    else  digitalWrite(MOTOR_PIN, LOW);

    //process display
    if (lastState != shouldShowResult) {
      if (shouldShowResult == true) {
        lastState = true;
        digitalWrite(HEARTBEAT_LED, HIGH);
        for (int i = 0; i <= 3; i++) {
          //turn on blinking digits by forcing them to re-paint
          if (displayStyles[i] == BLINK) {
            bitSet(displayChanged, i);
          }
        }
      } else {
        //turn off blinking digits
        digitalWrite(HEARTBEAT_LED, LOW);
        lastState = false;

        for (int i = 0; i <= 3; i++)
        {
          if (displayStyles[i] == BLINK) {
            mydisplay.setChar(0, i, ' ', false);
            mydisplay.setChar(0, i + 4, ' ', false);
          }
        }

      }
    }

    for (int i = 0; i <= 3; i++)
    {
      if ((bitRead(displayChanged, i) == true))
      {
        bitSet(displayChanged, i) = false;

        switch (displayColours[i])
        {
          case RED:
            mydisplay.setChar(0, i, displayCharacters[i], false);
            mydisplay.setChar(0, i + 4, ' ', false);
            break;
          case YELLOW:
            mydisplay.setChar(0, i, displayCharacters[i], false);
            mydisplay.setChar(0, i + 4, displayCharacters[i], false);
            break;
          case GREEN:
            mydisplay.setChar(0, i, ' ', false);
            mydisplay.setChar(0, i + 4, displayCharacters[i], false);
            break;
        }//switch
      }
    }
  }
}

boolean shouldShow() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= BLINK_RATE) {

    previousMillis = currentMillis;

    if (lastBlink == true)
    {
      lastBlink = false;
      return (false);
    }
    else
    {
      lastBlink = true;
      return (true);
    }
  }
  return (lastBlink);
}

boolean shouldBlank() {
  long currentMillis = millis();
  if ((currentMillis - lastDisplayUpdate > BLANK_DELAY) && (programMode == RUN) && (flightMode != LANDING_START) )
  {
    return (true);
  }
  else {
    return (false);
  }
}

//This function updates the display buffers with the string provided
void setDisplay(char text[], int display, int colour, int style) {

  byte change = 0;

  if (display == LEFT) {
    if (displayCharacters[0] != text[0]) {
      displayCharacters[0] = text[0];
      bitSet(change, 0);
    }
    if (displayCharacters[1] != text[1]) {
      displayCharacters[1] = text[1];
      bitSet(change, 1);
    }
    if (displayColours[0] != colour) {
      displayColours[0] = colour;
      bitSet(change, 0);
    }
    if (displayColours[1] != colour) {
      displayColours[1] = colour;
      bitSet(change, 1);
    }
    if (displayStyles[0] != style) {
      displayStyles[0] = style;
      bitSet(change, 0);
    }
    if (displayStyles[1] != style) {
      displayStyles[1] = style;
      bitSet(change, 1);
    }
  }

  if (display == RIGHT) {
    if (displayCharacters[2] != text[0]) {
      displayCharacters[2] = text[0];
      bitSet(change, 2);
    }
    if (displayCharacters[3] != text[1]) {
      displayCharacters[3] = text[1];
      bitSet(change, 3);
    }
    if (displayColours[2] != colour) {
      displayColours[2] = colour;
      bitSet(change, 2);
    }
    if (displayColours[3] != colour) {
      displayColours[3] = colour;
      bitSet(change, 3);
    }
    if (displayStyles[2] != style) {
      displayStyles[2] = style;
      bitSet(change, 2);
    }
    if (displayStyles[3] != style) {
      displayStyles[3] = style;
      bitSet(change, 3);
    }
  }
  noInterrupts();
  displayChanged = change;
  interrupts();
  lastDisplayUpdate = millis();
}

void chirp(int numberOfBeeps) {
  for (int i = 0; i <= numberOfBeeps; i++) {
    sounderOn = true;
    delay(400);
    sounderOn = false;
    delay(200);

    beepBuffer.push(SHORT_BEEP);
  }
}
