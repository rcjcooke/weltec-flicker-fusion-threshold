#include <Arduino.h>

// Surprised this definition doesn't exist in the ESP IDF
#define ADC1_CH0 36

/************************
 * Constants
 ************************/

// Pin definitions
static const uint8_t LED_PIN = 18;
static const uint8_t BUTTON_PIN = 16;
#if ESP_PLATFORM
static const uint8_t POTENTIOMETER_PIN = ADC1_CH0;
#else
static const uint8_t POTENTIOMETER_PIN = A3;
#endif
// Delay before accepting button state change for debouncing purposes
static const unsigned long LOCKOUT_DELAY_MILLIS = 2;

// The minimum period of time for which the LED will blink on in microseconds
static const unsigned long MIN_HALF_PERIOD_MICROS = 5000;
// The maximum period of time for which the LED will blink on in microseconds
static const unsigned long MAX_HALF_PERIOD_MICROS = 20000;

/************************
 * Variables
 ************************/
// True if the button has been pressed but not actioned
volatile bool gButtonStateChangeToAction = false;
// The debounced button state
uint8_t gButtonState = LOW;

// The time in microseconds that the LED is currently on or off
unsigned long gHalfPeriod = 100000;
// The last time we changed the state of the LED in system microseconds
unsigned long gLastLEDChangeMicros = 0;
// The current state of the LED
uint8_t gLEDState = LOW;

/*********************
 * Interrupt routines
 *********************/
void buttonPressedISR() {
  unsigned long currentMillis = millis();
  cli();
  static unsigned long lastButtonStateChangeTime = 0;

#if ESP_PLATFORM
  uint8_t newButtonState = digitalRead(BUTTON_PIN);
#else
  uint8_t newButtonState = digitalReadFast(BUTTON_PIN);
#endif

  // Make sure we're not in the lockout period
  if (currentMillis - lastButtonStateChangeTime > LOCKOUT_DELAY_MILLIS) {
    // Only do anything if we've changed state
    if (gButtonState != newButtonState) {
      lastButtonStateChangeTime = currentMillis;
      gButtonStateChangeToAction = true;
      gButtonState = newButtonState;
    }
  }
  sei();
}

/*********************
 * Entry Point methods
 *********************/
// Setup function - executes once on startup
void setup() {
  // Set up pins and internals
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(POTENTIOMETER_PIN, INPUT);
  analogReadResolution(10);
  analogSetAttenuation(ADC_6db);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPressedISR, CHANGE);
  digitalWrite(LED_PIN, gLEDState);
  Serial.begin(115200);
}

// Loop function - loops :)
void loop() {
  // Flash the LED
  unsigned long curMicros = micros();
  if (curMicros - gLastLEDChangeMicros > gHalfPeriod) {
    gLEDState = !gLEDState;
    digitalWrite(LED_PIN, gLEDState);
    gLastLEDChangeMicros = curMicros;
  }

  // Check the potentiometer
  int rawPotValue = analogRead(POTENTIOMETER_PIN);

#if ESP_PLATFORM
  gHalfPeriod = map((long) rawPotValue, 0, 1023, (long) MIN_HALF_PERIOD_MICROS, (long) MAX_HALF_PERIOD_MICROS);
#else
  gHalfPeriod = map<int, int, int, unsigned long, unsigned long>(rawPotValue, 0, 1023, MIN_HALF_PERIOD_MICROS, MAX_HALF_PERIOD_MICROS);
#endif

  // Check to see if the user has pressed the button
  if (gButtonStateChangeToAction && gButtonState == HIGH) {
    // Print current frequency
    float frequency = (float) 500000 / ((float) gHalfPeriod);
    Serial.println("Flicker fusion threshold frequency: " + String(frequency) + "Hz");
    gButtonStateChangeToAction = false;
  }
}
