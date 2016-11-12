/* This file takes care of power down
 *  Switch     on pin 43    (input, active low)
 *  Power down on pin 42    (output, active high)
 */

 
// Function to initialize the buzzer
//
void initPower(void)
{
  pinMode(__SW_SENSE, INPUT);
  digitalWrite(__PWR_DOWN, LOW);
  pinMode(__PWR_DOWN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(__SW_SENSE), switchISR, CHANGE);
  Serial.println("Initializing switch");
}

void switchISR(void)
{
  // Get if powerDown button has been pressed (TRUE) or released (FALSE)
  powerButton = !digitalRead(__SW_SENSE);
  if (powerButton)
  {
    // power button has been pressed
    powerDownMillis = millis();
  }
}

// Routine executed for soft power down.
// Routine is run in the loop, only if the power button is help pressed
// It contains programmed steps leading to power down
// Last, the __PWR_DOWN pin is set HIGH to turn off the power supply
// Once the __PWR_DOWN pin is pressed it takes a few seconds for a complete power down.

void powerDown(void)
{
  if ((millis() - powerDownMillis) > 2000)
  {
    // Beep at 2 kHz for 100 ms
    beep(2000, 100);
    // Wait 150 ms
    delay(150);
    Beep at 1kHz for 100 ms
    beep(1000, 100);
    // Set the power button FALSE, as if it is not held anymore
    powerButton = false;
    // Set __PWR_DOWN button HIGH to turn off the power.
    digitalWrite(__PWR_DOWN, HIGH);
  }
}


