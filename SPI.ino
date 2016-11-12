void initSPI(byte pin, byte divider)
{
  SPI.begin(pin);
  SPI.setClockDivider(divider);
}
