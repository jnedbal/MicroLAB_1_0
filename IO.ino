/* IO.ino manages the I/O pin control registers
 *  It sets which pins are inputs, which are outputs and
 *  sets the default values for outputs
 */


// Function to initialize the pins required as outputs
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
