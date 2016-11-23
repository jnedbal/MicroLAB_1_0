/* I2C.ino manages the I2C protocol data transfer.
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
 
 // Initialize I2C communication
void initI2C(void)
{
  // Start talk on Wire 1
  Wire1.begin();
}

// Function to take control of the I2C bus, it takes the bus away from the LCD shield
void startI2C(void)
{
  // Tell Micro to stop talking on the I2C port
  sendLCDcommand(stopI2Ccomm, LCDcontrol);  // control command
  // Wait two milliseconds to finish off any potential transfer
  delay(2);
  // Enable the RTC level shifter to gain access to the 5V devices
  digitalWrite(__I2C_EN, LOW);
}

// Function to release the I2C bus
void stopI2C(void)
{
  // Tell Micro to stop talking on the I2C port, it returns the bus to the LCD shield
  sendLCDcommand(startI2Ccomm, LCDcontrol);  // control command
  // Disable the RTC level shifter to release access to the 5V devices
  digitalWrite(__I2C_EN, HIGH);
  // Wait ten microseconds to allow the Arduino Micro to process the LCD command.
  delayMicroseconds(10);
}


