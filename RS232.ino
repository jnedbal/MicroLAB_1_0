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
  // Retrieve the data from the NVRAM at address 0x07
  retrieveNVdata(0x00, 0x00, brightnessContrastAddress, 0x02);

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
  // Store four bytes into address 0x07 in the NVRAM
  storeNVdata(0x00, 0x00, brightnessContrastAddress, 0x02);
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


// Function returning information about the selected filter.
// The filter positions are stored in a byte array, where each
// byte represents the chosen filter in each filter wheel.
// The software supports up to 10 filters per wheel. The filter
// selected in each filter wheel is returned to the computer.
void filterGet(void)
{
  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
  // Create a 4 byte array to hold the active filter positions
  byte sendBuffer[4];
  for (i = 0; i < servoCount; i++)
  {
    sendBuffer[servoActive[i]] = filterActive[i];
  }
  // Send to the PC
  SerialUSB.write(sendBuffer, 4);
}

// Function to move a particular filter wheel into a specified filter position
// The data sent by the computer consists of two bytes, the first byte specifies
// which of the four filter wheels is being addressed and the second byte
// specifies which of the ten possible positions to move the filter wheel into.
// The filter positions are stored in an array of uint16_t. The numbers stored
// within this array determine the PWM duty cycle driving the servos.
// The selected filter name is printed on the alphanumeric display.
// If the current filter position is not known for any of the filter wheels
// display error 8 on the screen.
// Store the selected filter combination into the NVRAM.

void filterSet(void)
{
  // serialbuffer[2]:  which wheel is used
  // serialbuffer[3]:  which position to go to
  // Move the filter wheel to the position addressed by the PC.
  // servoAddress[]     : array holding the Arduino DUE pin number tied to eacd servo driving the filter wheel
  // filterPosition[][] : PWM setting driving a given filter wheel to the specified position
  moveServo(servoAddress[serialbuffer[2]], filterPosition[serialbuffer[2]][serialbuffer[3] - 1]);
  // Print the name of the selected filter on the alphanumeric display.
  // Each filter position can be assigned up to three characters that are loaded onto the LCD.
  // filterNameMaxChar[] : specifies how many characters long are the names of given filter wheel
  // filterName[][][]    : string array specifying the filter wheel, filter position name characters
  for (i = 0; i < filterNameMaxChar[serialbuffer[2]]; i++)
  {
    // Print characters on LCD
    loadLCDdata(filterNameLCD[serialbuffer[2]] + i, filterName[serialbuffer[2]][serialbuffer[3] - 1][i]);
  }
  // Set the active filter position to the filter addressed by the computer.
  filterActive[serialbuffer[2]] = serialbuffer[3];

  // Store the existing error status
  byte err_old = err;
  // Assume there is no error in setting the filter wheel positions
  err &= 0b11110111;
  // Check if any of the filter wheel positions are set to zero, which indicates a problem
  for (i = 0; i < servoCount; i++)
  {
    if (filterActive[i] == 0)
    {
      // Set the filter wheel loading error
      err |= 0b00001000;
      // Set the error event
      ev2 |= 0b01000000;
    }
  }
  // If there is an error, display the error
  if (err != err_old)
  { 
    displayError();
  }

  // Update the log
  // Filter wheel position are stored as two bytes.
  // First byte fw12 holds first and second filter wheel positions in 4 LSb and 4 MSb, respectively
  // Second byte fw34 holds third and fourth filter wheel positions in 4 LSb and 4 MSb, respectively
  if (serialbuffer[2] < 2)
  {
    fw12 = (filterActive[0] | (filterActive[1] << 4));
  }
  else
  {
    fw34 = (filterActive[2] | (filterActive[3] << 4));
  }
  // Store the status into the NVRAM
  callEvent();
  
  // Display the buffer onto the LCD
  //updateLCD();

  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
}


// Function to update the filter look-up-table
void filterUpdate(void)
{
  servoSetting();
  // Update filter look up table
  ev1 = ((ev1 & 0b11110000) | NVbuffer[8]);
  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
}


// Function to set a filter in a filter wheel into an arbitrary position.
// The filter is set in three bytes.
// The first bytes determines which filter wheel is being addressed.
// The second and third byte specify the position which the filter wheel is meant to move into.
// Because the filter position set is arbitrary, remove selected filter from the LCD, 
void filterGoto(void)
{
  // serialbuffer[2]:    which wheel is used
  // serialbuffer[3-4]:  which position to go to, consists of a 16bit number between 700 and 2300
  // Go to the position sent by the computer
  moveServo(servoAddress[serialbuffer[2]], word(serialbuffer[3], serialbuffer[4]));
  
  // Switch on the servo only temporarily to prevent jumping of the servo
  //if (!servos[serialbuffer[2]].attached())
  {
    //servos[serialbuffer[2]].attach(servoAddress[serialbuffer[2]]);
  }
  //servos[serialbuffer[2]].writeMicroseconds(word(serialbuffer[3], serialbuffer[4]));

  // Delete filter position name from the LCD
  for (i = 0; i < filterNameMaxChar[serialbuffer[2]]; i++)
  {
    // Print space characters on LCD instead of the filter name
    loadLCDdata(filterNameLCD[serialbuffer[2]] + i, 32);
  }

  // Set the active filter position to zero (unknown position)
  filterActive[serialbuffer[2]] = 0;

  // Set error on the display as we are moving to arbitrary position
  err |= 0b00001000;
  ev2 |= 0b01000000;

  displayError();

  // Update the log
  // Filter wheel position are stored as two bytes.
  // First byte fw12 holds first and second filter wheel positions in 4 LSb and 4 MSb, respectively
  // Second byte fw34 holds third and fourth filter wheel positions in 4 LSb and 4 MSb, respectively
  if (serialbuffer[2] < 2)
  {
    fw12 = (filterActive[0] | (filterActive[1] << 4));
  }
  else
  {
    fw34 = (filterActive[2] | (filterActive[3] << 4));
  }
  // Store the status into the NVRAM
  callEvent();

  //delay(1000);
  //servos[serialbuffer[2]].detach();
  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
}

// Function to read the current event count stored in the NVRAM
// Ecount is a 16-bit integer that ranges up to 5449, before the
// NVRAM overflows.
void EcountGet(void)
{
  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
  // Send to the PC. First send the MSB then the LSB
  SerialUSB.write((byte) (Ecount >> 8) & 0xFF);
  SerialUSB.write((byte) Ecount & 0xFF);
}

// Function to transfer all NVRAM content to the computer, up to the last event.
void transferEvents(void)
{
  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
  // Do the bulk transfer
  // Read instruction
  SPI.transfer(__NVRAM, readInstr, SPI_CONTINUE);
  // Pass on address, which is 0x000000 in this case
  SPI.transfer(__NVRAM, 0x00, SPI_CONTINUE);
  SPI.transfer(__NVRAM, 0x00, SPI_CONTINUE);
  SPI.transfer(__NVRAM, 0x00, SPI_CONTINUE);
  // Pass on the data byte-by-byte, don't transfer the last byte
  for (i = 0x01; i < Eaddr; i++)
  {
    SerialUSB.write(SPI.transfer(__NVRAM, 0x00, SPI_CONTINUE));
  }
  // Transfer the last byte and finish the burst read from the NVRAM
  SerialUSB.write(SPI.transfer(__NVRAM, 0x00));
}

// Function to clear the entire event space of the NVRAM.
// The event counter and event address is set to the beginning of the NVRAM event space
// The event space of the NVRAM is set fo 0x00 throughout.
void wipeEvents(void)
{
  // Reset the Ecount index and the event memory
  // Event counter
  Ecount = 0;
  // Event address
  Eaddr = 0x200;
  // Write zeros to the whole of the Event space of the memory
  // Write instruction
  SPI.transfer(__NVRAM, writeInstr, SPI_CONTINUE);
  // Pass on address
  SPI.transfer(__NVRAM, 0x00, SPI_CONTINUE);
  SPI.transfer(__NVRAM, 0x02, SPI_CONTINUE);
  SPI.transfer(__NVRAM, 0x00, SPI_CONTINUE);
  // Sent zeros to each byte of the memory event space
  for (i = 0x200; i < 0x1FFFF; i++)
  {
    SPI.transfer(__NVRAM, 0x00, SPI_CONTINUE);
  }
  SPI.transfer(__NVRAM, 0x00);
  // Call an event to get something into the memory
  callEvent();
  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
}

// Function to store the pump Device name and serial number into the NVRAM
void pumpID(void)
{
  // Transfer the 7 bytes of pump ID into NVbuffer
  for (i = 0; i < 7; i++)
  {
    NVbuffer[i] = serialbuffer[i + 2];
  }
  // Store the pump ID into the NVRAM
  // Pump ID if 7-bytes long
  storeConstant(0x00, 0x00, pumpAddress, 0x07);
  // Create a pump event
  ev2 |= 0b00000100;
  callEvent();
  // Display 'p=' on the screen
  loadLCDdata(40, 112);
  loadLCDdata(41, 61);
  // Display '000' on the screen
  loadLCDdata(42, 48);
  loadLCDdata(43, 48);
  loadLCDdata(44, 48);
  // Display 'mb' on the screen
  loadLCDdata(45, 109);
  loadLCDdata(46, 98);

  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
}


// Function to store the pump pressure into the NVRAM
// The pump pressure is a 16-bit integer sent in two bytes.
void pumpPressure(void)
{
  // Create a pump pressure event
  ev2 |= 0b00000100;
  // Store the pump pressure into the NVRAM
  fp1 = serialbuffer[2];
  fp2 = serialbuffer[3];
  callEvent();
  // Combine the two bytes into a 16-bit integer
  uint16_t intNumber = word(serialbuffer[2], serialbuffer[3]);
  // Print the pump pressure converted into BCD digit by digit
  for (i = 0; i < 3; i++)
  {
    loadLCDdata(44 - i, HEXASCII[intNumber % 10]);
    intNumber /= 10;
  }
  // Return checksum and reset serial transfer
  checkSum();
  Sin = 0;
  serlen = 0;
}
