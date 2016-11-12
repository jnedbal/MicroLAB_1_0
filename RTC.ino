// Function to initialize the RTC
void initRTC(void)
{ 
  startI2C();

  // Give the but a millisecond to clear out any existing transmission
  delay(1);

  // Initialize the RTC
  rtc.begin();
  
  // Initialize the internal RTC
  rtc_clock.init();

  // Read the current time from DS1338
  DateTime now = rtc.now();

  // Wait, until a second increments on the RTC. Only then load it.
  // This minimizes the jitter on the internal clock, in respect 
  // to the RTC.
  now = rtc.now();              // Retrieve current time in the RTC
  byte sec_old = now.second();  // Store the original second
  byte sec = now.second();      // Store the current second

  // Loop while original and current second remain the same
  while (sec == sec_old)
  {
    now = rtc.now();            // Retrieve current time in the RTC
    sec = now.second();         // Store the current second
  }

  // Update time and date of the internal RTC to the DB1338 RTC
  rtc_clock.set_time(now.hour(), now.minute(), now.second());
  rtc_clock.set_date(now.day(), now.month(), now.year());

  // Make sure external RTC is not giving errors
  RTCerrorCheck();
  stopI2C();
}


//void setTimeDate(void)
//{
//  startI2C();
//  // following line sets the RTC to the date & time this sketch was compiled
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//    DateTime now = rtc.now();
//    
//    Serial.print(now.year(), DEC);
//    Serial.print('/');
//    Serial.print(now.month(), DEC);
//    Serial.print('/');
//    Serial.print(now.day(), DEC);
//    Serial.print(" (");
//    Serial.print(") ");
//    Serial.print(now.hour(), DEC);
//    Serial.print(':');
//    Serial.print(now.minute(), DEC);
//    Serial.print(':');
//    Serial.print(now.second(), DEC);
//    Serial.println();
//  delay(10);
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//  delay(10);
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//  delay(10);
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//  delay(10);
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//  delay(10);
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//  delay(10);
//  delay(5000);
//  stopI2C();
//  
//  // following line sets the RTC to the date & time this sketch was compiled
//  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//  //stopI2C();
//}

//
//void initRTC(void)
//{
//  // Start I2C communication on bus Wire1
//  Wire1.begin();
//  // Start communication with DS1338
//  rtc.begin();
//
//  // Initialize the internal RTC
//  rtc_clock.init();
//  // Start the second interrupt
//  //NVIC_EnableIRQ(RTC_IRQn);
//  //RTC_EnableIt(RTC, RTC_IER_SECEN);
//  // Read the current time from DS1338
//  DateTime now = rtc.now();
//  // Update time and date of the internal RTC
//  rtc_clock.set_time(now.hour(), now.minute(), now.second());
//  rtc_clock.set_date(now.day(), now.month(), now.year());
//  // Attach an interrupt function for every second tick
//  rtc_clock.attachsec(secondInterrupt);
//  // Make sure external RTC is not giving errors
//  RTCerrorCheck();
//}


void RTCerrorCheck(void)
{
  if (! rtc.isrunning())
  {
    // If the RTC is not running because it has seen power down
    err = err | B1;
    // Store event
    //callEvent();
  }
  else
  {
    err = err & B11111110;
  }

  /*if (rtc.oscstopflag())
  {
    // If the RTC has had problems with the oscillator
    // See OSF on page 11 of DS1338 datasheet for details
    err = err | B10;
    // Store event
    //callEvent();
  }
  else
  {
    err = err & B11111101;
  }*/
  // Update the error message on the display 
  displayError();
}
