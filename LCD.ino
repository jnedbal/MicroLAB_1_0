/*
  LiquidCrystal Library - Hello World
 
 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the 
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.
 
 This sketch prints "Hello World!" to the LCD
 and shows the time.
 
  The circuit:
 * LCD RS pin to digital pin 14
 * LCD Enable pin to digital pin 17
 * LCD D4 pin to digital pin 15
 * LCD D5 pin to digital pin 18
 * LCD D6 pin to digital pin 16
 * LCD D7 pin to digital pin 19
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 
 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
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


//void initLCD(void)
//{
//  /* Set correct pins to outputs */
//  pinMode(14, OUTPUT);    // LCD RS pin
//  pinMode(15, OUTPUT);    // LCD Enable
//  pinMode(16, OUTPUT);    // LCD D4
//  pinMode(17, OUTPUT);    // LCD D5
//  pinMode(18, OUTPUT);    // LCD D6
//  pinMode(19, OUTPUT);    // LCD D7
//  
//  loadLCDdata(2, 58);   // Add a colon between hour and minute
//  loadLCDdata(5, 58);   // Add a colon between minute and second
//  loadLCDdata(11, 47);  // Add a slash between day and month
//  loadLCDdata(14, 47);  // Add a slash between month and year
//  
//  // set up the LCD's number of columns and rows: 
//  lcd.begin(20, 4);
//}

//void printTime(void)
//{
//  //lcd.setCursor(0, 0);
//  uint32_t curtime = rtc_clock.current_time();
//  //lcd.print((curtime >> 20) & B11);
//  loadLCDdata(0, HEXASCII[(curtime >> 20) & B11]);
//  //lcd.print((curtime >> 16) & B1111);
//  loadLCDdata(1, HEXASCII[(curtime >> 16) & B1111]);
//  //lcd.print(":");
//  //lcd.print((curtime >> 12) & B111);
//  loadLCDdata(3, HEXASCII[(curtime >> 12) & B111]);
//  //lcd.print((curtime >> 8) & B1111);
//  loadLCDdata(4, HEXASCII[(curtime >> 8) & B1111]);
//  //lcd.print(":");
//  //lcd.print((curtime >> 4) & B111);
//  loadLCDdata(6, HEXASCII[(curtime >> 4) & B111]);
//  //lcd.print((curtime) & B1111);
//  loadLCDdata(7, HEXASCII[(curtime) & B1111]);
//  //lcd.print(" ");
//  uint32_t curdate = rtc_clock.current_date();
//  //lcd.print((curdate >> 28) & B11);
//  loadLCDdata(9, HEXASCII[(curdate >> 28) & B11]);
//  //lcd.print((curdate >> 24) & B1111);
//  loadLCDdata(10, HEXASCII[(curdate >> 24) & B1111]);
//  //lcd.print("/");
//  //lcd.print((curdate >> 20) & B1);
//  loadLCDdata(12, HEXASCII[(curdate >> 20) & B1]);
//  //lcd.print((curdate >> 16) & B1111);
//  loadLCDdata(13, HEXASCII[(curdate >> 16) & B1111]);
//  //lcd.print("/");
//  //lcd.print((curdate >> 12) & B1111);
//  loadLCDdata(15, HEXASCII[(curdate >> 12) & B1111]);
//  //lcd.print((curdate >> 8) & B1111);
//  loadLCDdata(16, HEXASCII[(curdate >> 8) & B1111]);
//
//  //lcd.setCursor(0, 0);
//  //lcd.print(LCD0);
//  updateLCD();
//
//}

//void updateLCD(void)
//{
//  while (LCDindex > 0)
//  {
//    byte tmpIn = LCDindex - 1;
//    lcd.setCursor(LCDnew[tmpIn] % 20, LCDnew[tmpIn] / 20);
//    lcd.write(LCDdata[LCDnew[tmpIn]]);
//    LCDindex--;
//  }
//}


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


// Function updates LCDdata register, LCDnew
// register and increments LCDindex
void loadLCDdata(byte index, byte data)
{
  LCDdata[LCDindexW] = data;
  LCDposition[LCDindexW] = index;
  LCDindexW++;
}

