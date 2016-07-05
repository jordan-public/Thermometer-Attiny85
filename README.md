# Thermometer-Attiny85

Attiny85 has an internal temperature sensor, which measures the chip temperature.
  However, if the chip is sleeping for a long time, the chip temperature would approach
  the ambient temperature.  
  This program puts Attiny85 to sleep and uses the Watchdog timer to wake it up after 
  about 9 seconds.  Then it measures the temperature and displays it by blinking the LED
  in Degrees Celsius as follows:
  - Negative sign is 4 quick blinks.
  - Each digit is a series of digit blinks separated by a pause.
    - Long blink means 0.
    - For all other digits the number of normal blinks corresponds to the
      digit displayed.  
  
  
The led should not be connected directly to the GPIO pin, but through a transistor with 1Mohm
resistor in the base circuit.  This minimizes the power drawn through the chip, to avoid
heating it above the ambient temperature, thus achieving better accuracy.  

I used an A42 NPN transistor for driving the LED with 220Ohm resistor in the collector circuit.
This is not the best choice - for a brighter LED a transistor with higher hFE should be used.

Notes: This works well with Vcc around 3.3V.  The compensation formula most likely needs correction,
in order to make the reading independent of the power supply voltage, so no volage regulator 
would be needed.  
