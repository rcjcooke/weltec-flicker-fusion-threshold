#include <Arduino.h>
#include <SevSeg.h>

// Surprised this definition doesn't exist in the ESP IDF
#define ADC1_CH0 36

/************************
 * Constants
 ************************/
// Pin definitions
#if ESP_PLATFORM
  static const uint8_t LED_PIN = 18;
  static const uint8_t POTENTIOMETER_PIN = ADC1_CH0;
  static const uint8_t BUTTON_PIN = 34;
#else
  static const uint8_t LED_PIN = 18;
  static const uint8_t POTENTIOMETER_PIN = A3;
  static const uint8_t BUTTON_PIN = 16;
#endif // ESP_PLATFORM
// For 7-Segment Display Pinout see:
// http://haneefputtur.com/7-segment-4-digit-led-display-sma420564-using-arduino.html
// Note: Can't use pin 32 at the moment so first digit is physically unconnected
// as 32 is assigned to 32KHz crystal on the Wrover Kit board.
static const uint8_t DISPLAY_1_PIN = 32;
static const uint8_t DISPLAY_2_PIN = 19;
static const uint8_t DISPLAY_3_PIN = 5;
static const uint8_t DISPLAY_4_PIN = 2;
static const uint8_t DISPLAY_A_PIN = 22;
static const uint8_t DISPLAY_B_PIN = 26;
static const uint8_t DISPLAY_C_PIN = 21;
static const uint8_t DISPLAY_D_PIN = 25;
static const uint8_t DISPLAY_E_PIN = 27;
static const uint8_t DISPLAY_F_PIN = 0;
static const uint8_t DISPLAY_G_PIN = 4;
static const uint8_t DISPLAY_DP_PIN = 23;

// Delay before accepting button state change for debouncing purposes
static const unsigned long LOCKOUT_DELAY_MILLIS = 100;

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
// Seven Segment display instance
SevSeg mSevenSegmentDisplay;

/*********************
 * Interrupt routines
 *********************/
#if ESP_PLATFORM
// ESP32 ISR Handlers MUST be placed in IRAM
// (https://esp-idf.readthedocs.io/en/v2.0/general-notes.html#iram-instruction-ram)
void IRAM_ATTR buttonPressedISR() {
#else
void buttonPressedISR() {
#endif // ESP_PLATFORM
  unsigned long currentMillis = millis();
  cli();
  static unsigned long lastButtonStateChangeTime = 0;

#if ESP_PLATFORM
  uint8_t newButtonState = digitalRead(BUTTON_PIN);
#else
  uint8_t newButtonState = digitalReadFast(BUTTON_PIN);
#endif // ESP_PLATFORM

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
#if ESP_PLATFORM
  analogSetAttenuation(ADC_6db);
#endif // ESP_PLATFORM
  byte numDigits = 4;
  byte digitPins[] = {DISPLAY_1_PIN, DISPLAY_2_PIN, DISPLAY_3_PIN,
                      DISPLAY_4_PIN};
  byte segmentPins[] = {DISPLAY_A_PIN, DISPLAY_B_PIN, DISPLAY_C_PIN,
                        DISPLAY_D_PIN, DISPLAY_E_PIN, DISPLAY_F_PIN,
                        DISPLAY_G_PIN, DISPLAY_DP_PIN};
  mSevenSegmentDisplay.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins, true);

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
  gHalfPeriod = map((long)rawPotValue, 0, 1023, (long)MIN_HALF_PERIOD_MICROS,
                    (long)MAX_HALF_PERIOD_MICROS);
#else
  gHalfPeriod = map<int, int, int, unsigned long, unsigned long>(
      rawPotValue, 0, 1023, MIN_HALF_PERIOD_MICROS, MAX_HALF_PERIOD_MICROS);
#endif // ESP_PLATFORM

  // Check to see if the user has pressed the button
  if (gButtonStateChangeToAction && gButtonState == HIGH) {
    // Print current frequency
    float frequency = (float)500000 / ((float)gHalfPeriod);
    Serial.println("Flicker fusion threshold frequency: " + String(frequency) +
                   "Hz");
    mSevenSegmentDisplay.setNumber(frequency, 1);
    gButtonStateChangeToAction = false;
  }
  mSevenSegmentDisplay.refreshDisplay();
}
