// PCA9685 16-channel PWM driver is on an I2C bus. It is being used to control the
// LCD brightness and contrast. It was also intended to drive the servos, this
// hoewever was incompatible with the contrast and brightness control as it made
// the display flicker. For this reason the servo control has been performed by the
// Arduino directly.
/*
#define backlightNum        9   // PWM channel for LCD brightness
#define contrastNum         0   // PWM channel for LCD contrast
#define defaultBacklight 1000   // Default value for LCD brightness
#define defaultContrast   400   // Default value for LCD contrast
#define LCDaddress       0x07   // NVRAM address for LCD brightness and contrast


void initPWM(void)
{
  // if you want to really speed stuff up, you can go into 'fast 400khz I2C' mode
  // some i2c devices dont like this so much so if you're sharing the bus, watch
  // out for this!
//  pwm.begin();
  // This is the maximum PWM frequency
//  pwm.setPWMFreq(1600);
  // Check if we have NVRAM error
  if ((err >> 2) & 0x01)
  {
    // If the NVRAM experienced a power-cut,
    // write default values to NVRAM memory
    NVbuffer[0] = (byte) ((defaultBacklight >> 8) & 0xFF);
    NVbuffer[1] = (byte) (defaultBacklight & 0xFF);
    NVbuffer[2] = (byte) ((defaultContrast >> 8) & 0xFF);
    NVbuffer[3] = (byte) (defaultContrast & 0xFF);
    // Store values starting at LCDaddress 
    storeConstant(LCDaddress, 0x04);
    // Update the event register to say that brightness and contrast has changed
    ev2 |= 0b00010000;
    // Store event
    callEvent();
    // Update the event register to say that brightness and contrast has NOT changed
    ev2 &= 0b11101111;
  }
  else
  {
    // Retrieve stored values from NVRAM memory starting at LCDaddress
    retrieveConstant(LCDaddress, 0x04);
  }
  // Set the brightness
  setLCDbrightness(word(NVbuffer[0], NVbuffer[1]));
  // Set the contrast
  setLCDcontrast(word(NVbuffer[2], NVbuffer[3]));
}

// Set the contrast of the LCD display.
// For the LMB204BFC display at room temperature,
// Value of 400 provided optimal contrast
void setLCDcontrast(uint16_t contrast)
{
  // make sure the value is in the 0-4095 range
  contrast = constrain(contrast, 0, 4095);
  // update the PWM
//  pwm.setPWM(contrastNum, 0, contrast);
}

// Set the brightness of the LCD display.
// Choose value between 0 and 4095
void setLCDbrightness(uint16_t brightness)
{
  // make sure the value is in the 0-4095 range
  brightness = constrain(brightness, 0, 4095);
  // update the PWM
//  pwm.setPWM(backlightNum, 0, brightness);
  
  NVbuffer[0] = (byte) ((brightness >> 8) & 0xFF);
  NVbuffer[1] = (byte) (brightness & 0xFF);
  //storeConstant(0x52, 0x01);
  NVbuffer[0] = 0x00;
  NVbuffer[1] = 0x00;
  //Serial.print("Reset NV buffer");
  //Serial.println(word(NVbuffer[0], NVbuffer[1]));
  //retrieveConstant(0x52, 0x01);
  //Serial.print("NV buffer updated from memory");
  //Serial.println(word(NVbuffer[0], NVbuffer[1]));
}
*/

