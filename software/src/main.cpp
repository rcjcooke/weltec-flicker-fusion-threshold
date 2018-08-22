#include <Arduino.h>

/************************
 * Constants
 ************************/
// Pin definitions
static const uint8_t LED_PIN = 18;
static const uint8_t BUTTON_PIN = 16;
static const uint8_t POTENTIOMETER_PIN = A3;

// Delay before accepting button state change for debouncing purposes
static const unsigned long DEBOUNCE_DELAY_MILLIS = 40;

// The minimum period of time for which the LED will blink on in microseconds
static const unsigned long MIN_HALF_PERIOD_MICROS = 5000;
// The maximum period of time for which the LED will blink on in microseconds
static const unsigned long MAX_HALF_PERIOD_MICROS = 20000;

/************************
 * Variables
 ************************/
// The last time the button interrupt was triggered
unsigned long gLastButtonStateChangeTime = 0;
// The last button state we wrote to the LED
volatile bool gButtonPressed = false;

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
  gLastButtonStateChangeTime = millis();
  gButtonPressed = true;
}

/*********************
 * Entry Point methods
 *********************/
// Setup function - executes once on startup
void setup() {
  // Set up pins and internals
  pinMode(LED_PIN, OUTPUT);
  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPressedISR, RISING);
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
  gHalfPeriod = map<int, int, int, unsigned long, unsigned long>(rawPotValue, 0, 1023, MIN_HALF_PERIOD_MICROS, MAX_HALF_PERIOD_MICROS);

  // Check to see if the user has pressed the button
  if (gButtonPressed && digitalReadFast(BUTTON_PIN) == HIGH) {
    // Make sure it's not a bounce
    if (millis() - gLastButtonStateChangeTime > DEBOUNCE_DELAY_MILLIS) {
      // Print current frequency
      float frequency = (float) 500000 / ((float) gHalfPeriod);
      Serial.println("Flicker fusion threshold frequency: " + String(frequency) + "Hz");
      gButtonPressed = false;
    }
  }
}
