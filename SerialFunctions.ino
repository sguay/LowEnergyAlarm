

/****************Serial Functions                           *****************/
void readNMEAData() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '$';
  char endMarker = '*';
  char rc;

  while (Serial5.available() > 0 && newData == false) {
    rc = Serial5.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }
    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

//http://forum.arduino.cc/index.php?topic=288234.60
void parseNMEAData() {

  /* split the data into its parts

    Condor Data Format from
    https://github.com/XCSoar/XCSoar/blob/master/src/Device/Driver/LX/Parser.cpp

    $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1
    0 loger_stored (Y/N)
    1 IAS (kph) ----> Condor uses TAS!
    2 baroaltitude (m)
    3-8 vario (m/s) (last 6 measurements in last second)
    9 heading of plane
    10 windcourse (deg)
    11 windspeed (kph)

  */

  char buf[10];
  char sentenceID[numChars] = {0};
  char * strtokIndx; // this is used by strtok() as an index

#ifdef DEBUG_SERIAL
  sprintf(buf, "Recieved Data: %s", tempChars);
  Serial.println(buf);
#endif

  strtokIndx = strtok(tempChars, ",");     // get the first part - the string
  strcpy(sentenceID, strtokIndx); // copy it to sentenceID

#ifdef DEBUG_SERIAL
  sprintf(buf, "SentenceID: %s", sentenceID);
  Serial.println(buf);
#endif

  if (strcmp(sentenceID, "LXWP0") == 0) {
//logger_stored (throw away) (Y/N)
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    strcpy(sentenceID, strtokIndx);
#ifdef DEBUG_SERIAL
    sprintf(buf, "One: %s", sentenceID); 
    Serial.println(buf);
#endif

//IAS (KPH)
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    strcpy(sentenceID, strtokIndx);
    airSpeed = atof(strtokIndx);     // convert this part to an integer
#ifdef DEBUG_SERIAL
    sprintf(buf, "Two: %s", sentenceID); 
    Serial.println(buf);
    sprintf(buf, "Airpeed: %f", airSpeed);
    Serial.println(buf);
#endif

//baroaltitude (m)
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    strcpy(sentenceID, strtokIndx);
    baroAltitude = atof(strtokIndx);     // convert this part to an integer
#ifdef DEBUG_SERIAL
    sprintf(buf, "Three: %s", sentenceID);
    Serial.println(buf);
    sprintf(buf, "Altitude: %f", baroAltitude);
    Serial.println(buf);
#endif
    lastValidData = millis();
//varo 3-8
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off

    //heading
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off

    //windcourse
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off

    //windspeed
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off

  } else
  {
    for (int i = 0; i <= 11; i++) {
      strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    }
  }

}

