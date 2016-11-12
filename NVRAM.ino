// Section NVRAM deals with the non-volatile
// RAM 23LCV1024 backed-up with a battery.

// Each time an event occurs a series of information is stored.
// Bytes 0 - 1 are the event number
// Bytes 2 - 4 are the date
// Bytes 5 - 7 are the time
// Bytes 8 - 11 are milliseconds since the last restart
// Byte 12 events 1:
//  7: updated servo 1 (filter wheel 1)
//  6: updated servo 2 (filter wheel 2)
//  5: updated servo 3
//  4: updated servo 4
//  3: updated servo 1 look-up table
//  2: updated servo 2 look-up table
//  1: updated servo 3 look-up table
//  0: updated servo 4 look-up table
// Byte 13 events 2:
//  7:  reset
//  6:  error
//  5:  updated time
//  4:  updated backlight and contrast
//  3:  updated shutters
//  2:  reserved
//  1:  reserved
//  0:  reserved
// Byte 14 events 3:
//  7:  reserved
//  6:  reserved
//  5:  reserved
//  4:  reserved
//  3:  reserved
//  2:  reserved
//  1:  reserved
//  0:  reserved
// Byte 15 is the error vector
//  7:  reserved
//  6:  reserved
//  5:  reserved
//  4:  reserved
//  3:  reserved
//  2:  NVRAM does not hold expected values in first four bytes,
//            suspected loss of power. All settings and logs lost.
//  1:  External RTC encountered an oscillator stop. Some problem with
//            power supply, noise or board layout. Update the time.
//  0:  External RTC was not running, power failure. Update the time.
// Byte 16 are servos 1 & 2 settings
//  7:  3 servo 2
//  6:  2 servo 2
//  5:  1 servo 2
//  4:  0 servo 2
//  3:  3 servo 1
//  2:  2 servo 1
//  1:  1 servo 1
//  0:  0 servo 1
// Byte 17 are servos 3 & 4 settings
//  7:  3 servo 4
//  6:  2 servo 4
//  5:  1 servo 4
//  4:  0 servo 4
//  3:  3 servo 3
//  2:  2 servo 3
//  1:  1 servo 3
//  0:  0 servo 3
// Byte 18 are shutter settings and LCD backlight
//  7:  LCD backlight ON
//  6:  shutter 4 ON
//  5:  shutter 3 ON
//  4:  shutter 2 ON
//  3:  shutter 1 ON
//  2:  reserved
//  1:  reserved
//  0:  reserved
// Byte 19 - 23 are reserved

// The first 256 bytes of the NVRAM are used to store settings 
// during power outage. The map of this memory spoace follows:

// 0x00      : memory check byte 0: 0x00;
// 0x01      : memory check byte 1: 0xFF;
// 0x02      : memory check byte 2: 0x55;
// 0x03      : memory check byte 3: 0xAA;
// 0x04-0x06 : event address
// 0x07-0x08 : LCD brightness
// 0x09-0x0A : LCD contrast


void initNVRAM(void)
{
  // Initialize SPI on pin 4, with frequency of 16.8 MHz (84 MHz / 5)
  initSPI(4, 5);

  // Read first four bytes of the NVRAM. They should contain:
  // address  value BIN    value HEX
  //  0x00    0b00000000     0x00
  //  0x01    0b11111111     0xFF
  //  0x02    0b01010101     0x55
  //  0x03    0b10101010     0xAA
  // If they don't it means that the memory has lost power,
  // indicated by error 0b100 
  retrieveConstant(0x00, 0x04);
  if ((NVbuffer[0] != 0x00) || (NVbuffer[1] != 0xFF) ||
      (NVbuffer[2] != 0x55) || (NVbuffer[3] != 0xAA))
  {
    // If the NVRAM does not have the first four bytes correct
    // suspect a power down and a failure
    err |= B100;
    // Set the first four bytes to the correct value for next time
    NVbuffer[0] = 0x00;
    NVbuffer[1] = 0xFF;
    NVbuffer[2] = 0x55;
    NVbuffer[3] = 0xAA;
    storeConstant(0x00, 0x04);
    // Store event
    //callEvent();
  }
  else
  {
    // Remove the NVRAM error flag
    err &= B11111011;
  }
  // Update the error message on the display 
  displayError();

  // Scan the NVRAM to find the last stored event in the log
  // start from address 0x100, and increment by 0x18.
  // Each time read two bytes, combine them into a word
  // Check that the word has increased by one fromt he last one
  // If it did not increase by one, you know that the end of
  // the log has been reached
  // If there is NVRAM error, there is no point of testing the memory
  // Start with Event 0 and Eaddr 0x100
  if ((err >> 2) & 0x01)
  {
    // Event counter
    Ecount = 0;
    // Event address
    Eaddr = 0x100;
  }
  else
  {
    uint32_t address = 0x100;
    uint16_t eventNrLast;
    uint16_t eventNr;
    boolean OK = true;
    eventNr = retrieveEventNr(address);
    //Serial.print("Eaddr: ");
    //Serial.println(Eaddr);
    //Serial.print("Event: ");
    //Serial.println(eventNr);
    while (OK | (address > 0x1FFE9)) // (0x20000-0x17)
    {
      address += 0x18;
      eventNrLast = eventNr;
      eventNr = retrieveEventNr(address);
      //Serial.print("Event: ");
      //Serial.println(eventNr);
      if (eventNr != eventNrLast + 1)
      {
        OK = false;
      }
    }
    //Serial.print("Eaddr: ");
    //Serial.println(address);
    //Serial.print("Event: ");
    //Serial.println(eventNrLast + 1);

    // Event counter
    Ecount = eventNrLast + 1;
    // Event address
    Eaddr = address;
  }
}

/* void testNVRAM(void)
{
  // Make sure the event address does not overflow the 
  // 1 Mbit memory space. Update the event address.
  Eaddr = Eaddr + 0x10;
  if (Eaddr == 0x20000)
  {
    Eaddr = 0x100;
  }
  unsigned long addr = 0x01A6BA;
  SPI.transfer(4, writeInstr, SPI_CONTINUE);
  delayMicroseconds(2);
  SPI.transfer(4, 0x01, SPI_CONTINUE);
  delayMicroseconds(2);
  SPI.transfer(4, 0xA6, SPI_CONTINUE);
  delayMicroseconds(2);
  SPI.transfer(4, 0xBA, SPI_CONTINUE);
  delayMicroseconds(2);
  SPI.transfer(4, 0x34);
    SPI.transfer(4, readInstr, SPI_CONTINUE);
  SPI.transfer(4, (byte) ((addr >> 16) & 0xFF), SPI_CONTINUE);
  SPI.transfer(4, (byte) ((addr >> 8) & 0xFF), SPI_CONTINUE);
  SPI.transfer(4, (byte) (addr & 0xFF), SPI_CONTINUE);
  byte answer = SPI.transfer(4, 0x00);
  // had to comment out, otherwise the LCD buffer filled up too quickly
  //loadLCDdata(42, HEXASCII[(answer >> 4) & B1111]);
  //loadLCDdata(43, HEXASCII[(answer) & B1111]);
  //loadLCDdata(45, HEXASCII[(0x34 >> 4) & B1111]);
  //loadLCDdata(46, HEXASCII[(0x34) & B1111]);
  //lcd.setCursor(2, 2);
  //lcd.print((byte) answer, HEX);
  //lcd.setCursor(5, 2);
  //lcd.print((byte) 0x34, HEX);
  //lcd.setCursor(2, 3);
  //lcd.print((byte) ((addr >> 8) & 0xFF), HEX);
  delay(10);
}*/


// This function is called each time a loggable event is recorder
void callEvent(void)
{
  // Make sure the event address does not overflow the 
  // 1 Mbit memory space. Update the event address.
  if (Eaddr > 0x1FFE9)  // Don't overflow the memory
  {
    Eaddr = 0x100;
  }
  //Serial.print("Ecount: ");
  //Serial.println(Ecount);
  //Serial.print("Eaddr: ");
  //Serial.println(Eaddr);
  // First two bytes contain the event number  
  Evector[0] = (byte) ((Ecount >> 8) & 0xFF);
  Evector[1] = (byte) (Ecount & 0xFF);
  // Next three bytes are the date
  uint32_t curtime = rtc_clock.current_date();
  Evector[2] = (byte) ((curtime >> 8) & 0xFF);
  Evector[3] = (byte) ((curtime >> 16) & 0xFF);
  Evector[4] = (byte) ((curtime >> 24) & 0xFF);
  // Next three bytes are the time
  curtime = rtc_clock.current_time();
  Evector[5] = (byte) ((curtime >> 16) & 0xFF);
  Evector[6] = (byte) ((curtime >> 8) & 0xFF);
  Evector[7] = (byte) (curtime & 0xFF);
  // Next four bytes are the milliseconds since last reset
  curtime = millis();
  Evector[8] = (byte) ((curtime >> 24) & 0xFF);
  Evector[9] = (byte) ((curtime >> 26) & 0xFF);
  Evector[10] = (byte) ((curtime >> 8) & 0xFF);;
  Evector[11] = (byte) (curtime & 0xFF);

  // Write instruction
  SPI.transfer(4, writeInstr, SPI_CONTINUE);
  // Pass on address
  SPI.transfer(4, (byte) ((Eaddr >> 16) & 0xFF), SPI_CONTINUE);
  SPI.transfer(4, (byte) ((Eaddr >> 8) & 0xFF), SPI_CONTINUE);
  SPI.transfer(4, (byte) (Eaddr & 0xFF), SPI_CONTINUE);
  for (i = 0; i < 23; i++)
  {
    SPI.transfer(4, Evector[i], SPI_CONTINUE);
  }
  SPI.transfer(4, Evector[23]);
  Eaddr = Eaddr + 0x18;    // Increment by 24 (DEC) or 0x18 (HEX)
  Ecount = Ecount + 1;
}

// This function is called to store a constant into the memory
// addr, is the address at which the writing should start
// nrBytes, how many bytes need to be written minus ONE
// data from a byte array called "NVbuffer" are transferred
void storeConstant(byte address, byte nrBytes)
{
  // Write instruction
  SPI.transfer(4, writeInstr, SPI_CONTINUE);
  // Pass on address
  SPI.transfer(4, 0x00, SPI_CONTINUE);
  SPI.transfer(4, 0x00, SPI_CONTINUE);
  SPI.transfer(4, address, SPI_CONTINUE);
  // Pass on the data byte-by-byte, don't transfer the last byte
  for (i = 0; i < nrBytes; i++)
  {
    SPI.transfer(4, NVbuffer[i], SPI_CONTINUE);
  }
  // Pass on the last data byte and terminate the transfer
  SPI.transfer(4, NVbuffer[nrBytes]);
}


// This function is called to retrieve a constant from the memory
// addr, is the address at which the reading should start
// nrBytes, how many bytes need to be read minus ONE
// data from the memory are stored into  byte array called "NVbuffer"
void retrieveConstant(byte address, byte nrBytes)
{
  // Write instruction
  SPI.transfer(4, readInstr, SPI_CONTINUE);
  // Pass on address
  SPI.transfer(4, 0x00, SPI_CONTINUE);
  SPI.transfer(4, 0x00, SPI_CONTINUE);
  SPI.transfer(4, address, SPI_CONTINUE);
  // Pass on the data byte-by-byte, don't transfer the last byte
  for (i = 0; i < nrBytes; i++)
  {
    NVbuffer[i] = SPI.transfer(4, 0x00, SPI_CONTINUE);
  }
  // Pass on the last data byte and terminate the transfer
  NVbuffer[nrBytes] = SPI.transfer(4, 0x00);
}


// This function is called to retrieve a constant from the memory
// addr, is the address at which the reading should start
// nrBytes, how many bytes need to be read minus ONE
// data from the memory are stored into  byte array called "NVbuffer"
uint16_t retrieveEventNr(unsigned long address)
{
  // Write instruction
  SPI.transfer(4, readInstr, SPI_CONTINUE);
  // Pass on address
  SPI.transfer(4, (byte) ((address >> 16) & 0xFF), SPI_CONTINUE);
  SPI.transfer(4, (byte) ((address >> 8) & 0xFF), SPI_CONTINUE);
  SPI.transfer(4, (address & 0xFF), SPI_CONTINUE);
  // Read two bytes
  uint16_t eventNr;
  eventNr = word(SPI.transfer(4, 0x00, SPI_CONTINUE), SPI.transfer(4, 0x00));
  return eventNr;
}
