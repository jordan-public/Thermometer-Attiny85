/*
  Thermometer Attiny85

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

  modified 2016-06-09
  by Jordan Stojanovski
 */

#include <avr/wdt.h>
#include <avr/sleep.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define CLOCK_DIVIDER_ADJUSTMENT

#define LED 1

#ifndef CLOCK_DIVIDER_ADJUSTMENT
#define SETTLE 100
#define PAUSE 1000
#define BLINK_DURATION 300
#define FAST_BLINK_DURATION 100
#else 
#define SETTLE 1000
#define PAUSE 10000
#define BLINK_DURATION 3000
#define FAST_BLINK_DURATION 1000
#endif

//****************************************************************
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;
  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}

//****************************************************************  
// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA,ADEN);  // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
  sleep_disable();              // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

void blinkInteger(int n)
{
  if (0 == n) { // Blink zero
    digitalWrite(LED, HIGH);
    delay(5 * BLINK_DURATION);
    digitalWrite(LED, LOW);
    delay(PAUSE);
  } else if (n < 0) { // Blink minus
    for (int i = 0; i<5; i++) { // fast blink 4 times
      digitalWrite(LED, HIGH);
      delay(FAST_BLINK_DURATION);
      digitalWrite(LED, LOW);
      delay(FAST_BLINK_DURATION);    
    }
    delay(PAUSE);
    blinkInteger(-n);
  } else if (n <= 9 && n >=1) {
    for (int i = 0; i<n; i++) {
      digitalWrite(LED, HIGH);
      delay(BLINK_DURATION);
      digitalWrite(LED, LOW);
      delay(BLINK_DURATION);    
    }
    delay(PAUSE);
  } else { // n >= 10
    int div;
    int digits = 0;
    for (div = 1; div <= n; div *= 10) digits++; // count the number of digits
    for (int i=0; i<digits; i++) {
      div /= 10;
      blinkInteger(n / div);
      delay(PAUSE);
      n %= div;
    }
  }
}

// From: https://learn.sparkfun.com/tutorials/h2ohno/low-power-attiny
// sleepTime:
//  0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
//  6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void sleep(int sleepTime) {
    setup_watchdog(sleepTime); //Setup watchdog to go off after: 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms, 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
    system_sleep(); //Go to sleep! Wake up 1sec later and check water
}


// From: http://21stdigitalhome.blogspot.com/2014/10/trinket-attiny85-internal-temperature.html
int getChipTemperatureCelsius() {
  int i;
  int t_celsius; 
  uint8_t vccIndex;
  float rawTemp, rawVcc;
  
  // Measure temperature
  ADCSRA |= _BV(ADEN);           // Enable AD and start conversion
  ADMUX = 0xF | _BV( REFS1 );    // ADC4 (Temp Sensor) and Ref voltage = 1.1V;
  delay(SETTLE);                 // Settling time min 1 ms, wait 100 ms

  rawTemp = (float)getADC();     // use next sample as initial average
  for (int i=2; i<2000; i++) {   // calculate running average for 2000 measurements
    rawTemp += ((float)getADC() - rawTemp) / float(i); 
  }  
  ADCSRA &= ~(_BV(ADEN));        // disable ADC  

  // Measure chip voltage (Vcc)
  ADCSRA |= _BV(ADEN);   // Enable ADC
  ADMUX  = 0x0c | _BV(REFS2);    // Use Vcc as voltage reference, 
                                 //    bandgap reference as ADC input
  delay(SETTLE);                    // Settling time min 1 ms, there is 
                                 //    time so wait 100 ms
  rawVcc = (float)getADC();      // use next sample as initial average
  for (int i=2; i<2000; i++) {   // calculate running average for 2000 measurements
    rawVcc += ((float)getADC() - rawVcc) / float(i);
  }
  ADCSRA &= ~(_BV(ADEN));        // disable ADC
  
  rawVcc = 1024 * 1.1f / rawVcc;
  //index 0..13 for vcc 1.7 ... 3.0
  vccIndex = min(max(17,(uint8_t)(rawVcc * 10)),30) - 17;   

  // Temperature compensation using the chip voltage 
  // with 3.0 V VCC is 1 lower than measured with 1.7 V VCC 
  t_celsius = (int)(chipTemp(rawTemp) + (float)vccIndex / 13);  
                                                                                   
  return t_celsius;
}

// Calibration of the temperature sensor has to be changed for your own ATtiny85
// per tech note: http://www.atmel.com/Images/doc8108.pdf
float chipTemp(float raw) {
  const float chipTempOffset = 272.9;           // Your value here, it may vary 
  const float chipTempCoeff = 1.075;            // Your value here, it may vary
  return((raw - chipTempOffset) / chipTempCoeff);
}
 
// Common code for both sources of an ADC conversion
int getADC() {
  ADCSRA  |=_BV(ADSC);           // Start conversion
  while((ADCSRA & _BV(ADSC)));    // Wait until conversion is finished
  return ADC;
}

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(LED, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  int temperatureCelsius = getChipTemperatureCelsius(); // this must be done before blinking the led, as it increases the chip temperature
  blinkInteger(temperatureCelsius);
  
  sleep(9); // 9= 8sec (see above in the sleep() definition
}
