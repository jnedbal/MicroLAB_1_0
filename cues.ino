/* This file controls the visual and sound cues. These consist of:
 *  LED    on pin 13
 *  Buzzer on pin 12
 */


// Function to sound the buzzer
//    frequency in Hertz (Hz)
//    duration  in millisecond (ms)
//
//  Uses Timer3
//
void beep(uint32_t frequency, uint32_t duration)
{
  // Half-period of calls in microseconds
  //    This defines period between toggling the buzzer pin
  Timer3.start(1e6 / frequency / 2);
  // Calculate how many toggles are to be performed within the specified duration time
  buzzerCycles = 2000 * duration * frequency / 1e6;
}



// Function toggling the buzzer pin, when Timer3 interrupt is called
//
void buzzerToggle(void)
{
  // If the number of buzzer pin toggles has been exceeded, turn off the buzzer.
  if (buzzerCycle > buzzerCycles)
  {
    Timer3.stop();      // Stop the timer
    __BUZZERoff;        // Turn the pin LOW
    buzzerOn = false;   // Note that buzzer pin is LOW
    buzzerCycle = 0;    // Reset the buzzer cycle counter
    return;
  }

  // Toggle the buzzer pin
  if (buzzerOn)
  {
    __BUZZERoff;        // Turn the pin LOW
  }
  else
  {
    __BUZZERon;         // Turn the pin HIGH
  }
  buzzerOn = !buzzerOn; // Toggle the buzzer state
  buzzerCycle += 1;     // Increment the buzzer cycle counter
}



// Function to initialize the buzzer
//
void initBuzzer(void)
{
  // Use Timer3 to control the buzzer by calling the "buzzerToggle" function
  Timer3.attachInterrupt(buzzerToggle);
  // Beep at 5kHz for 100ms
  beep(5000, 100);
}

