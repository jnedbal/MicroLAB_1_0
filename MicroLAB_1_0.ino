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
 * The communication is done via a par<zallel 8-bit port, two address
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


// Error vector as the 16th byte of the Event vector
#define err Evector[15]
// Event 1 vector as the 13th byte of the Event vector
#define ev1 Evector[12]
// Event 2 vector as the 14th byte of the Event vector
#define ev2 Evector[13]

// Direct port manipulation for fast communication with the LCD daughter board
//                    ........|...AA..|.....CDDDDDDDD. 
#define LCDchar     0b00000000000001000000000000000000
#define LCDcontrol  0b00000000000000000000000000000000
#define LCDCS       0b00000000000000000000001000000000
#define LCDclear    0b00000000000011000000000111111110

// Definitions for LCD daughter board communication
#define resetLCD      1
#define startI2Ccomm  2
#define stopI2Ccomm   3
#define initializeDAC 11
#define brightnessMSB 12
#define brightnessLSB 13
#define contrastMSB   14
#define contrastLSB   15

// Define Arduino DUE pins
#define __I2C_EN      22    // I2C bus voltage translator
#define __LED         13    // LED
#define __BUZZER      12    // Buzzer
#define __SW_SENSE    A7    // Power switch sense (active low) Should be normally pin 43
#define __PWR_DOWN    42    // Power down system (active high)

// Define pin functions
#define __LEDon       PIOB->PIO_SODR=1<<27
#define __LEDoff      PIOB->PIO_CODR=1<<27
#define __BUZZERon    PIOD->PIO_SODR=1<<8
#define __BUZZERoff   PIOD->PIO_CODR=1<<8


// Wait one instruction
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


// Identifier for MicroFLiC
char ID[] = "MicroFLiC v1.1 ArduinoDUE (Jan 16, 2015)";

// Old error vector
byte olderr;
byte t;

/* Variables for NVRAM */
// Event counter
unsigned long Ecount = 0;
// Event address
unsigned long Eaddr = 0x100;
// Event vector
byte Evector[24];

// LCD lines data storage
byte LCDdata[256];
byte LCDposition[256];
byte LCDindexW;
byte LCDindexR;

unsigned long LCDdataBuffer;
unsigned long LCDoldBuffer;

// NVRAM transfer buffer
byte NVbuffer[8];

// counting index
int i;

boolean ledOn = true;

// 34 byte long buffer storing serial data
byte serialbuffer[102];
// Serial FIFO index
byte Sin = 0;
// Serial transfer length (up to 32 bytes) plus 2.
// For transfer of 4 bytes, serlen should be 4+2=6.
byte serlen = 0;

// Variables for running the buzzer
boolean buzzerOn;
uint32_t buzzerCycle;
uint32_t buzzerCycles;

// Variables for power control
boolean powerButton = false;
uint32_t powerDownMillis;

/********************/
/* Define constants */
/********************/

// Days of the week
const char* daynames[]={"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

/* Constants for NVRAM */
const byte writeInstr = 0x02;
const byte readInstr = 0x03;
/* End of constants for NVRAM */

/* Constants for LCD */
// lookup table for HEX numbers
const byte HEXASCII[16] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 59, 70};

void setup()
{
  // Initialize the IO pins
  initPins();
  // Initialize the buzzer
  initBuzzer();
  // Start serial communication of programming port
  Serial.begin(115200);
  SerialUSB.begin(115200);
  // Initialize the power switch
  initPower();
  // Initialize the LCD
  initLCDpins();
  // Initialize error. Create an error character
  initError();

  // Initialize I2C communication
  initI2C();
  // Initialize the RTCs
  initRTC();
  //pinMode(13, OUTPUT);
  //pinMode(2, OUTPUT);
  //digitalWrite(13, LOW);
  // Initialize non-volatile RAM
  initNVRAM();
  delay(100);
  //testNVRAM();
  // Initialize the PWM
  //initPWM();
  // Attach an interrupt updating the clock
  //Timer3.attachInterrupt(printTime).setFrequency(1).start();
  // Call reset event
  ev2 = ev2 | B10000000;
  callEvent();
  ev2 = ev2 & B01111111;
}


// Function to initialize the pins required for communication with the LCD
void initPins()
{
  initLCDpins();
  pinMode(__LED, OUTPUT);       // LED
  __LEDoff;                     // turn LED off
  pinMode(__BUZZER, OUTPUT);    // buzzer
  __BUZZERoff;                  // turn buzzer off
  digitalWrite(__I2C_EN, HIGH); // I2C bus voltage translator disabled
  pinMode(__I2C_EN, OUTPUT);    // I2C bus voltage translator (active LOW)
}


void loop() {
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
  //rs232loop();
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
  delay(100);
  // If power button is pressed, check if power needs to be turned off
  if (powerButton)
  {
    powerDown();
  }
}
