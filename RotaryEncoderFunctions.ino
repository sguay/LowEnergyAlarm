
/****************Rotary Encoder Functions                   *****************/
void timerIsr() {
  encoder->service();
}

int getEncoderValue() {

  value += encoder->getValue();

  int result = value - last;
  if (value != last) {

  }
  last = value;

  return (result);
}

int getClick() {

  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print(b);
    //#define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      //VERBOSECASE(ClickEncoder::Pressed)
      //VERBOSECASE(ClickEncoder::Released)
      //VERBOSECASE(ClickEncoder::Open)
      case ClickEncoder::Held:
        return (HELD);
        break;
      case ClickEncoder::Clicked:
        return (CLICKED);
        break;
      case ClickEncoder::DoubleClicked:
        return (DOUBLECLICKED);
        break;
    }
    
  }

}

