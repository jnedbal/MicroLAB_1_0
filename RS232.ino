/* Function takes care of communicating with the computer over USB
 *  The attempt for communication happens continually by calling rs232loop
 *  function from the loop() function. It listens for any available data on
 *  the Native port of the Arduino DUE. If a transfer occurs, it reads, 
 *  interprets and processes the data. The transfer occurs in packets. 
 *  The first byte contains the length of the transfer.
 *  The second byte contains the command.
 *  The remaining bytes are the data themselves.
 *  Typically, after the data has been received, a checksum of the packet
 *  is returned back to the computer for error checking.
 */

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

void setTimeDate(void)
{
  // Return checksum
  checkSum();
  Sin = 0;
  serlen = 0;

  // Arrays to hold strings of time and date
  char ctime[8];     // time (HH:MM:SS)
  char cdate[11];    // date (mmm dd yyyy)

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

  // Gain control over the I2C bus
  startI2C();

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

  // Return the I2C bus to the LCD shield
  stopI2C();

  // Update the event register 2 to say that time has changed
  ev2 |= 0b00100000;

  // Store event
  callEvent();

  // Update the event register to say that time NOT changed anymore
  ev2 &= 0b11011111;
}

// Function that sends current time and date as it is stored in the Internal RTC back to the computer.
void getTimeDate(void)
{
  // Create a 6-byte buffer to send the time and date
  uint8_t buffer[6];
  // Get the current time
  uint32_t curtime = rtc_clock.current_time();
  // Wait, until the second changes, to sent the time and the turn of the second.
  while (curtime == rtc_clock.current_time())
  {
    
  }
  // Read the new time with the currently incremented second
  curtime = rtc_clock.current_time();
  // Read the current date
  uint32_t curdate = rtc_clock.current_date();
  // Form the current time and date into the six bytes of the buffer
  buffer[0] = (curtime >> 16) & 0xFF;
  buffer[1] = (curtime >> 8) & 0xFF;
  buffer[2] = curtime & 0xFF;
  buffer[3] = (curdate >> 24) & 0xFF;
  buffer[4] = (curdate >> 16) & 0xFF;
  buffer[5] = (curdate >> 8) & 0xFF;
  
  // Return checksum
  checkSum();
  Sin = 0;
  serlen = 0;
  // Return the time and date to the computer.
  SerialUSB.write(buffer, 6);
}

// Send the device ID to the computer. The device ID is defined in this program.
void getID(void)
{
  // Return checksum
  checkSum();
  Sin = 0;
  serlen = 0;
  // First send the expected length of the transfer. How many bytes of ID, compilation date and time.
  SerialUSB.write(sizeof(ID));
  SerialUSB.write(sizeof(__DATE__));
  SerialUSB.write(sizeof(__TIME__));
  // Then send the device ID, the compilation date and time.
  SerialUSB.print(ID);
  SerialUSB.print(__DATE__);
  SerialUSB.print(__TIME__);
}

// Send the LCD brightness and contrast to the computer.
// The LCD brightness and contrast are stored in the EEPROM of the DAC
// but also in bytes 0x07-0x08 of the NVRAM. The NVRAM is read and data
// is transferred into the computer
void getLCDcontrastBrightness(void)
{
  retrieveNVdata(0x00, 0x00, 0x07, 0x02);

  // Return checksum
  checkSum();
  Sin = 0;
  serlen = 0;
  // Send back the two bytes of data with the brightness and contrast setting.
  SerialUSB.write(NVbuffer, 2);
}

// Set the LCD brightness and contrast.
// Store the LCD brightness and contrast in the EEPROM of the DAC
// but also in bytes 0x07-0x08 of the NVRAM.
void setLCDcontrastBrightness(void)
{
  // Store last four bytes into NVRAM buffer
  for (i = 0; i < 2; i++)
  {
    NVbuffer[i] = serialbuffer[i + 2];
  }
  // Save four bytes into address 0x07 in the NVRAM
  storeNVdata(0x00, 0x00, 0x07, 0x02);
  // Set the brightness
  LCDbrightness(NVbuffer[0]);
  // Set the contrast
  LCDcontrast(NVbuffer[1]);

  // Update the event register
  ev2 |= 0b00010000;
  callEvent();

  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
}

// Function to store packets of data to be stored in the NVRAM. The original aim was
// to program the filter wheel positions into the NVRAM. These settings program the PWM
// duty cycle, which in turn controls the servos. The data is transferred over the
// Native USB port in the following format. The first two bytes specify the address
// at which data is written in the NVRAM. The remaining up to 60 bytes contain the PWM
// settings to be stored in the NVRAM. This data represents the servo positions for the
// individual filters.
void packetStore(void)
{
  // Data comes in four packet of maximum 64 bytes, they need
  // to be stored into NVbuffer and stored into the NVRAM

  // store up to 60 bytes into NVbuffer
  for (i = 4; i < serialbuffer[0]; i++)
  {
    NVbuffer[i - 4] = serialbuffer[i];
  }

  // Store into NVRAM
  storeNVdata(0x00, serialbuffer[2], serialbuffer[3], serialbuffer[0] - 4);

  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
}

// Function to retrieve packets with data store in the NVRAM. Originally, this function
// was intended to read data with the filter wheel positional settings from the NVRAM.
// These settings program the PWM duty cycle, which in turn controls the servo positions.
// The data is requested over the Native USB port in the following format. The first two
// bytes specify the address at which data is retrieved from the NVRAM. The third byte
// specifies, how many bytes of data needs to be retrieved from the NVRAM. In response,
// the function sends back the requested data over the USB bus.
void packetSend(void)
{
  retrieveNVdata(0x00, serialbuffer[2], serialbuffer[3], serialbuffer[4]);
  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
  SerialUSB.write(NVbuffer, serialbuffer[4]);
}
