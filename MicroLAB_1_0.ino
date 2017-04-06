/* Code for Arduino DUE
 * This code runs the MicroLab. It controls a number of elements:
 *  o Shutters and valves
 *  o Servos
 *  o Liquid stirrer
 *  o LCD display
 *  o Real time clock
 *  o NVRAM backup
 *  o Quad 16-bit DAC
 * This code tests the LCD display connected to the microlabduino.
 * It requires the shield to be installed, The LCD shield with 
 * Arduino/Genuino Micro and LCD installed and 12 V power attached.
 * The communication is done via a parallel 8-bit port, two address
 * bits and a chip select signal. It consists of one way
 * communication from the Arduino DUE to Arduino/Genuino Micro.
 * 
 */
 
/*******************/
/* Include headers */
/*******************/
// include the library for internal RTC
#include <rtc_clock.h>
// include the library to communicate with DS1338
#include <RTClib.h>
// include the library for using timers.
#include <DueTimer.h>
// the NVRAM communicates using SPI, so include the library:
#include <SPI.h>
// I2C library is needed for communication with peripherals
#include <Wire.h>


/************************/
/*  #define Directives  */
/************************/


/* Error and Event handling directives */
// Error vector as the 16th byte of the Event vector
#define err Evector[15]
// Event 1 vector as the 13th byte of the Event vector
#define ev1 Evector[12]
// Event 2 vector as the 14th byte of the Event vector
#define ev2 Evector[13]
// Servo 1&2 position
#define fw12 Evector[16]
// Servo 3&4 position
#define fw34 Evector[17]
// ON/OFF vector as the 19th byte of the Event vector
#define onoff Evector[18]
// Fluika pump pressure MSB
#define fp1 Evector[19]
// Fluika pump pressure LSB
#define fp2 Evector[20]


/* NVRAM address directives */
// Dividing factor for one ninth of the NVRAM full
#define oneNinth 0x38E4
// LCD brightness/contrast setting NVRAM address
#define brightnessContrastAddress 0x07
// The pump ID and pressure address
#define pumpAddress 0x0B
// The filter wheel setting start address
#define servoSetAddress 0x80
// The filter wheel pre-programmed position start address
#define servoLUTaddress 0x9A


/* I2C transmission directives */
//    Make sure this address is unique on the I2C bus and identical in the stirrer
#define SlaveDeviceId 9
// Number of bytes in I2C transfer to stirrer
#define lengthI2Cbuf 14

/* Direct port manipulation for fast communication with the LCD daughter board */
//                    ........|...AA..|.....CDDDDDDDD. 
#define LCDchar     0b00000000000001000000000000000000
#define LCDcontrol  0b00000000000000000000000000000000
#define LCDCS       0b00000000000000000000001000000000
#define LCDclear    0b00000000000011000000000111111110


/* LCD daughter board communication directives */
#define resetLCD      1
#define startI2Ccomm  2
#define stopI2Ccomm   3
#define initializeDAC 11
#define brightnessMSB 12
#define brightnessLSB 13
#define contrastMSB   14
#define contrastLSB   15


/* Arduino DUE pins directives */
#define __I2C_EN      22    // I2C bus voltage translator
#define __LED         13    // LED
#define __BUZZER      12    // Buzzer
#define __SW_SENSE    A7    // Power switch sense (active low) Should be normally pin 43
#define __PWR_DOWN    42    // Power down system (active high)
#define __NVRAM       52    // SPI chip select pin for NVRAM


/* Pin function directives */
#define __LEDon       PIOB->PIO_SODR=1<<27
#define __LEDoff      PIOB->PIO_CODR=1<<27
#define __BUZZERon    PIOD->PIO_SODR=1<<8
#define __BUZZERoff   PIOD->PIO_CODR=1<<8


/* Wait one instruction */
#define NOP __asm__ __volatile__ ("nop\n\t")



/********************/
/*  Define objects  */
/********************/


// Start the DS1338 RTC communication
RTC_DS1307 rtc;

// Start the internal RTC
RTC_clock rtc_clock(XTAL);




/********************/
/* Define variables */
/********************/


/* Identifier for MicroFLiC */
char ID[] = "MicroLAB v1.0 ArduinoDUE (Nov 20, 2016)";

/* Counting indices */
int i;
uint16_t loopCycle;


// Old error vector
//byte olderr;
//byte t;



/* Variables for NVRAM */
// Event counter
unsigned long Ecount = 0;
// Event address
unsigned long Eaddr = 0x200;
// Event vector
byte Evector[24];
// NVRAM transfer buffer
byte NVbuffer[256];
// Memory fillup character
byte RAMfill[] = {0x4D, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xFF};

/* LCD lines data storage */
byte LCDdata[256];
byte LCDposition[256];
boolean LCDcharpos;   // false: send character; true: send position
byte LCDindexW;
byte LCDindexR;

unsigned long LCDdataBuffer;
unsigned long LCDoldBuffer;






// boolean ledOn = true;

/* Serial data transfer variables */
// 102 byte long buffer storing serial data
byte serialbuffer[102];
// Serial FIFO index
byte Sin = 0;
// Serial transfer length (up to 100 bytes) plus 2.
// For transfer of 4 bytes, serlen should be 4+2=6.
byte serlen = 0;
/* End of serial data transfer variables */


/* Variables for running the buzzer */
// Variables keeping track of the buzzer. Buzzer is operated by Timer3.
// It is a piezo sounder connected to pin 12 via a MOSFET
// Is buzzer on?
boolean buzzerOn;
// Buzzer period counting index
uint32_t buzzerCycle;
// Total number of buzzer periods to play
uint32_t buzzerCycles;
/* End of buzzer variables */


/* Variables for power control */
// Has the power button been pressed?
boolean powerButton = false;
// Variable recording the time the button was pressed
uint32_t powerDownMillis;
/* End of power control variables */



/********************/
/* Define constants */
/********************/

/* Days of the week */
const char* daynames[]={"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

/* Constants for NVRAM */
const byte writeInstr = 0x02;
const byte readInstr = 0x03;
/* End of constants for NVRAM */

/* Constants for LCD */
// lookup table for HEX numbers
const byte HEXASCII[16] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 59, 70};

// String inString = "";    // string to hold input




void setup()
{
  /* Initialize pins */
  // Initialize buzzer and LED pins
  initCuesPins();
  // Initialize I2C bus control pin
  initI2CPins();
  // Initialize the LCD control pins
  initLCDpins();


  // Initialize the IO pins
  // initPins();


  /* Give audible cue of system start by sounding the buzzer */
  // Sound the buzzer at 5 kHz for 100 ms
  initBuzzer();

  /* Start communication over USB */
  // Start serial communication on the programming port
  Serial.begin(115200);
  // Start serial communication on the native port
  SerialUSB.begin(115200);

  /* Initialize different sub modules */
  // Initialize the power switch
  initPower();

  // Initialize error. Create an error character
  initError();

  // Initialize I2C communication
  initI2C();

  // Initialize the RTCs
  initRTC();

  // Initialize non-volatile RAM
  //Serial.println("NVRAM:");
  initNVRAM();

  /* Wait 100 ms before anything else happens */
  delay(100);
  //testNVRAM();
  // Initialize the PWM
  //initPWM();
  // Attach an interrupt updating the clock
  //Timer3.attachInterrupt(printTime).setFrequency(1).start();

  /* Record in the event memory that the device has been restarted */
  // Call reset event
  ev2 = ev2 | B10000000;
  callEvent();
  ev2 = ev2 & B01111111;
  
  loadLCDdata(20, HEXASCII[6]);
  loadLCDdata(21, HEXASCII[7]);
  loadLCDdata(22, HEXASCII[8]);
  /*loadLCDdata(33, HEXASCII[1]);
  loadLCDdata(33, HEXASCII[2]);
  loadLCDdata(33, HEXASCII[3]);
  loadLCDdata(33, HEXASCII[4]);
  loadLCDdata(33, HEXASCII[5]);
  loadLCDdata(33, HEXASCII[6]);
  loadLCDdata(33, HEXASCII[7]);
  loadLCDdata(33, HEXASCII[8]);
  loadLCDdata(33, HEXASCII[9]);
  loadLCDdata(33, HEXASCII[9]);*/
}




void loop()
{
  // Read serial input:
//  while (Serial.available() > 0) {
//    int inChar = Serial.read();
//    if (isDigit(inChar)) {
//      // convert the incoming byte to a char
//      // and add it to the string:
//      inString += (char)inChar;
//    }
//    // if you get a newline, print the string,
//    // then the string's value:
//    if (inChar == 'b')
//    {
//      LCDbrightness((byte) inString.toInt());
//      Serial.print("Brightness: ");
//      Serial.println(inString);
//      // clear the string for new input:
//      inString = "";
//    }
//    if (inChar == 'c')
//    {
//      LCDcontrast((byte) inString.toInt());
//      Serial.print("Contrast: ");
//      Serial.println(inString);
//      // clear the string for new input:
//      inString = "";
//    }
//  }
  //Serial.print("Error? ");
  //Serial.println(err, BIN);
  //Serial.print("Is OK? ");
  //Serial.println(rtc_clock.current_time(),DEC);
//  DateTime now = rtc.now();
//  Serial.print("Hour: ");
//  Serial.println(now.hour(), DEC);
//  Serial.print("Minute: ");
//  Serial.println(now.minute(), DEC);
//  Serial.print("Second: ");
//  Serial.println(now.second(), DEC);
//  delay(1000);
  //printTime();

  /* Read next available characters on the Native USB port */
  rs232loop();

  //testNVRAM();
  //digitalWrite(13, digitalRead(13)?LOW:HIGH);
  //t++;
  //Serial.print(t, DEC);
  //Serial.print(" / 10 = ");
  //Serial.println(t / 10, DEC);
  //Serial.print(t, DEC);
  //Serial.print(" % 10 = ");
  //Serial.println(t % 10, DEC);
//  Serial.println(rtc_clock.current_time(), HEX);
//  Serial.print("At the third stroke, it will be ");
//  Serial.print(rtc_clock.get_hours());
//  Serial.print(":");
//  Serial.print(rtc_clock.get_minutes());
//  Serial.print(":");
//  Serial.println(rtc_clock.get_seconds());
//  Serial.print(daynames[rtc_clock.get_day_of_week()-1]);
//  Serial.print(": ");
//  Serial.print(rtc_clock.get_days());
//  Serial.print(".");
//  Serial.print(rtc_clock.get_months());
//  Serial.print(".");
//  Serial.println(rtc_clock.get_years());

  /* Update the LCD display if needed */
  // Send LCD character about every 12 ms
  loopCycle++;
  if (!(loopCycle % 0x1000))
  {
    transferLCDdata();
  }
  
  //transferLCDdata();
  /* Control the power button */
  // Power down if power button held pressed long enough
  if (powerButton)
  {
    powerDown();
  }
}
