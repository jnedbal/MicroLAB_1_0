/* SPI.ino controls the SPI data transfer.
 *  The Arduino DUE supports three SPI devices on the following pins.
 *  4   Quad-channel 16-bit DAC
 *  10  External SPI bus
 *  52  Non-volative RAM
 */

// Initialize the SPI transfer.
//  pin     can be 4, 10 or 52
//  divider is an integer setting SPI data transfer frequency derived from the 84 MHz clock.
//          For instance divider of 84, sets 1 MHz SPI bus transfer speed.
//
void initSPI(byte pin, byte divider)
{
  SPI.begin(pin);
  SPI.setClockDivider(divider);
}
