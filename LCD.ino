/* LCD.ino manages the data transfer to the LCD shield.
 *  The communication is over an 8-bit bus with two address pins and
 *  and active-low chip select (CS) pin. It is a one-way data bus.
 *  The Arduino Micro has no way of communicating to the Arduino DUE.
 *  
 *  There are a number of functions contained in this script:
 *    # It sets the Arduino DUE pins for the communication with the shield.
 *    # It sends data to the Arduino Micro.
 *    # It buffers any data needed to be displayed on the LCD.
 *    # It sends buffered data onto the LCD.
 *    # It sets brightness and contrast of the LCD.
 */


// Initialize pins for LCD operation
void initLCDpins(void)
{
  /* signal  pin  port
   *   D0    33   PC1
   *   D1    34   PC2
   *   D2    35   PC3
   *   D3    36   PC4
   *   D4    37   PC5
   *   D5    38   PC6
   *   D6    39   PC7
   *   D7    40   PC8
   *   CS    41   PC9
   *   A0    45   PC18
   *   A1    44   PC19
   */
   digitalWrite(33, LOW);   // D0
   digitalWrite(34, LOW);   // D1
   digitalWrite(35, LOW);   // D2
   digitalWrite(36, LOW);   // D3
   digitalWrite(37, LOW);   // D4
   digitalWrite(38, LOW);   // D5
   digitalWrite(39, LOW);   // D6
   digitalWrite(40, LOW);   // D7
   digitalWrite(41, HIGH);  // CS
   digitalWrite(45, LOW);   // A0
   digitalWrite(44, LOW);   // A1

   pinMode(33, OUTPUT);     // D0
   pinMode(34, OUTPUT);     // D1
   pinMode(35, OUTPUT);     // D2
   pinMode(36, OUTPUT);     // D3
   pinMode(37, OUTPUT);     // D4
   pinMode(38, OUTPUT);     // D5
   pinMode(39, OUTPUT);     // D6
   pinMode(40, OUTPUT);     // D7
   pinMode(41, OUTPUT);     // CS
   pinMode(45, OUTPUT);     // A0
   pinMode(44, OUTPUT);     // A1
}

void sendLCDcommand(uint32_t BYTE, uint32_t ADDRESS)
{
  /* This function sends over two bytes of data, the POSition and CHARacter
   * Both are 7-bit numbers. The 8th bit determines if POSition or CHARacter 
   * is being sent. The sequence is:
   * 1. set chip select bit HIGH
   * 2. clear bus: 8-bits of LCD data and the address
   * 3. send 8 bits of LCD data on parallel bus
   * 4. set the LCD address
   * 5. bring chip select bit ZERO to trigger interrupt
   */

  // 1. set chip select bit HIGH
  PIOC->PIO_SODR = LCDCS;

  // 2. clear bus: 8-bits of LCD data and the address
  PIOC->PIO_CODR = LCDclear;

  // 3. send 8 bits of LCD data on parallel bus
  PIOC->PIO_SODR = (BYTE << 1);

  // 4. set the LCD address
  PIOC->PIO_SODR = ADDRESS;

  // 5. bring chip select bit ZERO to trigger interrupt
  PIOC->PIO_CODR = LCDCS;
}


// Function updates LCDdata register, LCDnew register and increments LCDindexW
// This function stores a character and position into the buffer for transfer into the LCD shield.
void loadLCDdata(byte index, byte data)
{
  LCDdata[LCDindexW] = data;
  LCDposition[LCDindexW] = index;
  LCDindexW++;
}

// Function that needs to be called repeatedly in the loop() of the script.
// It tests whether there is new data in the LCD buffer.
// If so, it transfers the position on the LCD in the first cycle
//        and the character in the second cycle.
void transferLCDdata(void)
{
//  Serial.print("LCDindexR: ");
//  Serial.print(LCDindexR);
//  Serial.print(";  LCDindexW: ");
//  Serial.print(LCDindexW);
//  Serial.print(";  LCDcharpos: ");
//  Serial.println(LCDcharpos);
  
  // exit function if there is no untransfered data in the buffer 
  if (LCDindexR == LCDindexW)
  {
    return;
  }
  // First transfer character (LCDcharpos==0), then transfer its position (LCDcharpos==1)
  if (LCDcharpos)
  {
//    Serial.print("pos: ");
//    Serial.println(LCDposition[LCDindexR]);
    // send the position of the character on the LCD 0-79
    sendLCDcommand(LCDposition[LCDindexR], LCDchar);
    // now that position is sent, send the character in the next call of the function
    LCDcharpos = false;
    // now that character is sent, move on to the next character in the buffer.
    LCDindexR++;
  }
  else
  {
//    Serial.print("char: ");
//    Serial.println(LCDdata[LCDindexR]);
    // send the character data, i.e. ASCII code 0-127
    sendLCDcommand(LCDdata[LCDindexR] | 0b10000000, LCDchar);
    // now that character is sent, prepare to send the position of the next character
    LCDcharpos = true;
  }
}

// Function to set LCD brightness
// The brightness is a 8-bit unsigned integer that is sent in two 4-bit transfers
// Brightness should be set so that it does not exceed the maximum permitted current through the LCD backlight.
// For Topway LMB204B display the permitted range of brightness is between 60 and 190.
void LCDbrightness(byte brightness)
{
  // send the 4 most significant bits of brightness
  sendLCDcommand((brightness & 0xF0) | brightnessMSB, LCDcontrol);
  // wait 10 us for the LCD shield to process them
  delayMicroseconds(10);
  // send the 4 least significant bits of brightness
  sendLCDcommand(((brightness & 0x0F) << 4) | brightnessLSB, LCDcontrol);
  
}

// Function to set LCD contrast
// The brightness is a 8-bit unsigned integer that is sent in two 4-bit transfers
// Contrast should be set so that it produces visible characters on the LCD.
// For Topway LMB204B display the working range of contrast is between 20 and 70 and optimum of 40.
void LCDcontrast(byte contrast)
{
  // send the 4 most significant bits of contrast
  sendLCDcommand((contrast & 0xF0) | contrastMSB, LCDcontrol);
  // wait 10 us for the LCD shield to process them
  delayMicroseconds(10);
  // send the 4 least significant bits of contrast
  sendLCDcommand(((contrast & 0x0F) << 4) | contrastLSB, LCDcontrol);
}

