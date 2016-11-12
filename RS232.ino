void checkSum(void)
{
  /* Checksum from
     https://github.com/alvieboy/arduino-oscope/blob/master/oscope.pde
     It makes bitwise XOR on the entire stack of data */
  byte cksum = 0;
  for (i = 0; i < Sin; i++)
  {
    cksum ^= serialbuffer[i];
  }
  SerialUSB.write(cksum);
  /* Octave code to calculate the checksum
  A=[repmat('0', 5, 1) dec2bin(double('Dqwer'))]
  W='        ';V=repmat(false,1,8);u=0;for i = 1:8;for j=1:5; u=u+1;V(i)=xor(V(i), logical(bin2dec(A(u))));end;end;for i=1:8; W(i)=dec2bin(V(i));end;W=bin2dec(W);char(W)
  */
}

void rs232loop(void)
{
  /*******************************************************/
  /* This routine should run in loop all the time.       */
  /* It listens to the serial port for instructions.     */
  /* The first byte it receives gives the number of      */
  /* bytes in the transmission packet.                   */
  /* The second byte is the instruction.                 */
  /* The remaining bytes are the data                    */
  /*******************************************************/

  if (!SerialUSB.available() > 0)
  {
    return;
  }
  // Get incoming byte and store it in the serial buffer.
  serialbuffer[Sin] = SerialUSB.read();
  // Increment the serial buffer index.
  Sin++;
  // Read the very first byte of the serial buffer.
  // It contains the length of the data transfer "serlen".
  if (Sin == 1)
  {
    serlen = serialbuffer[0];
  }
  // Once the entire length of the packet has been received, process it.
  if (Sin == serlen)
  {
    switch (serialbuffer[1])      // the second byte contains the command
    {
      /* if the command is G (71 in ASCII) for Get time */
      case 71:
        getTimeDate();
        break;

      /* if the command is I (73 in ASCII) for ID */
      case 73:
        getID();
        break;

      /* if the command is L (76 in ASCII) for LCD */
      case 76:
        getLCDcontrastBrightness();
        break;

      /* if the command is C (67 in ASCII) for LCD */
      case 67:
        setLCDcontrastBrightness();
        break;

      /* if the command is S (83 in ASCII) for Set time */
      case 83:
        setTimeDate();
        break;
    }
  }
}

void setTimeDate()
{
  // Return checksum
  checkSum();
  Sin = 0;
  serlen = 0;
  // Arrays to hold strings of time and date
  char ctime[8];     // time (HH:MM:SS)
  char cdate[11];   // date (mmm dd yyyy)
  // load bytes 3 to 9 containing current time into ctime
  for(i = 2; i < 10; i++)
  {
    ctime[i - 2] = (char) serialbuffer[i];
  }
  // load bytes 10 to 20 containing current date into cdate
  for(i = 10; i < serlen; i++)  // loop through bytes 3 to 9 of the array
  {
    cdate[i - 10] = (char) serialbuffer[i];
  }
  // Update the external RTC
  rtc.adjust(DateTime(cdate, ctime));
  // Recall the time from the external RTC
  DateTime now = rtc.now();
  // Update time and date of internal RTC
  rtc_clock.set_time(now.hour(), now.minute(), now.second());
  rtc_clock.set_date(now.day(), now.month(), now.year());
  // Update the oscillator stop flag to 0.
  // It requires setting bit 5 in register 0x07 to zero.
  rtc.writenvram(0x07, rtc.readnvram(0x07) & B11011111);
  // Make sure external RTC is not giving errors
  RTCerrorCheck();

  // Update the event register 2 to say that time has changed
  ev2 |= 0b00100000;
  // Store event
  callEvent();
  // Update the event register to say that time NOT changed anymore
  ev2 &= 0b11011111;
}

void getTimeDate(void)
{
  checkSum();
  Sin = 0;
  serlen = 0;
  uint32_t curtime = rtc_clock.current_time();
  uint32_t curdate = rtc_clock.current_date();
  uint8_t buffer[6];
  buffer[0] = (curtime >> 16) & 0xFF;
  buffer[1] = (curtime >> 8) & 0xFF;
  buffer[2] = curtime & 0xFF;
  buffer[3] = (curdate >> 24) & 0xFF;
  buffer[4] = (curdate >> 16) & 0xFF;
  buffer[5] = (curdate >> 8) & 0xFF;
  SerialUSB.write(buffer, 6);
}

void getID(void)
{
  checkSum();
  Sin = 0;
  serlen = 0;
  SerialUSB.write(sizeof(ID));
  SerialUSB.write(sizeof(__DATE__));
  SerialUSB.write(sizeof(__TIME__));
  SerialUSB.print(ID);
  SerialUSB.print(__DATE__);
  SerialUSB.print(__TIME__);
}

void getLCDcontrastBrightness(void)
{
  checkSum();
  Sin = 0;
  serlen = 0;
  retrieveConstant(0x07, 0x04);
  SerialUSB.write(NVbuffer, 4);
}

void setLCDcontrastBrightness(void)
{
  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
  // Store last four bytes into NVRAM buffer
  for (i = 0; i < 4; i++)
  {
    NVbuffer[i] = serialbuffer[i + 2];
  }
  // Save four bytes into address 0x07 in the NVRAM
  storeConstant(0x07, 0x04);
  // Set the brightness
  setLCDbrightness(word(NVbuffer[0], NVbuffer[1]));
  // Set the contrast
  setLCDcontrast(word(NVbuffer[2], NVbuffer[3]));
}
