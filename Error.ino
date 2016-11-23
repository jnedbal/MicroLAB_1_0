/*********************************************/
/* Functions dealing with errors of the unit */
/*********************************************/
//
// The error code is stored in an 8 bit variable err
// Each bit signalizes an error. If any error occurs
// It will be printed on the display in the top right
// corner (coordinate 17, 0).The error codes are 
// summarized below:
// 00000001:  External RTC was not running, power
//            failure. Update the time.
// 00000010:  External RTC encountered an oscillator
//            stop. Some problem with power supply,
//            noise or board layout. Update the time.
// 00000100:  NVRAM does not hold expected values in
//            first four bytes, suspected loss of 
//            power supply. All settings and logs lost.


void initError(void)
{
  err = 0x00;
  //createErrorChar();
}



void displayError(void)
{
  if (err)
  {
    // If error has occurred, print it on the display
    loadLCDdata(17, byte(0));
    loadLCDdata(18, HEXASCII[(err >> 4) & B1111]);
    loadLCDdata(19, HEXASCII[err & B1111]);
  }
  else
  {
    // If no error is recorded, clear the last three characters of the first line
    loadLCDdata(17, byte(32));
    loadLCDdata(18, byte(32));
    loadLCDdata(19, byte(32));
  }
}
