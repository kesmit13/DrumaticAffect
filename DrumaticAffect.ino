//
// Drumatic Affect
//
// This code is for a device that drives a Digitech SDRUM pedal in order
// to set a specific tempo, select the song part to start with and
// optionally start the song. This is done by driving relays that emulate
// the pedals on a footswitch. One relay drives a pedal that duplicates
// the footswitch on the pedal itself. The other relay sends signals to
// the FSX3 input.
//
// This code is based on the code at https://www.instructables.com/Arduino-Metronome/.
// Metronome, Leading Accent, Visual&Audible Tact - 2019 Peter Csurgay
//
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBoldOblique12pt7b.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define PIN_A 2
#define PIN_B 3
#define TAP_START_PIN 4
#define TAP_PIN 5
#define KICKOFF_PIN 6
#define IDLE_11 0
#define SCLK_01 1
#define SCLK_00 2
#define SCLK_10 3
#define SDT_10 4
#define SDT_00 5
#define SDT_01 6
int state = IDLE_11;
int bpm = 90;
int curVal = 0;
int prevVal = 0;

unsigned long lastChangeTime = 0;
unsigned long changeDelay = 100;

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 150;    // the debounce time; increase if the output flickers

void setup() {
  pinMode(TAP_PIN, OUTPUT);
  pinMode(KICKOFF_PIN, OUTPUT);
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(TAP_START_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_A), rotaryCLK, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_B), rotaryDT, CHANGE);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();
  updateDisplay();
  //Serial.begin(9600);
  //while (!Serial);
}

void rotaryCLK() {
  if (digitalRead(PIN_A)==LOW) {
    if (state==IDLE_11) state = SCLK_01;
    else if (state==SCLK_10) state = SCLK_00;
    else if (state==SDT_10) state = SDT_00;
  }
  else {
    if (state==SCLK_01) state = IDLE_11;
    else if (state==SCLK_00) state = SCLK_10;
    else if (state==SDT_00) state = SDT_10;
    else if (state==SDT_01) { state = IDLE_11; curVal--; }
  }
}

void rotaryDT() {
  if (digitalRead(PIN_B)==LOW) {
    if (state==IDLE_11) state = SDT_10;
    else if (state==SDT_01) state = SDT_00;
    else if (state==SCLK_01) state = SCLK_00;
  }
  else {
    if (state==SDT_10) state = IDLE_11;
    else if (state==SDT_00) state = SDT_01;
    else if (state==SCLK_00) state = SCLK_01;
    else if (state==SCLK_10) { state = IDLE_11; curVal++; }
  }
}

void updateDisplay() {
    display.clearDisplay();
    display.setFont(&FreeMonoBoldOblique12pt7b);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor((bpm >= 100) ? 18 : 33, 45);
    display.print(bpm);
    display.display();
}

void blink() {
  int nTaps = 3;
  int tapDuration = 20;
  // https://planetcalc.com/8110/ - 0.91 @ 60, 0.81 @ 130
  double fudge = -0.0014285714285714284 * bpm + 0.9957142857142858;
  int wait = int(fudge * 1000. * 60. / bpm - tapDuration);
  for (int i = 0; i < nTaps; i++) {
    digitalWrite(TAP_PIN, HIGH);
    display.drawCircle(display.width() / 2, display.height() / 2, 32, SSD1306_INVERSE);
    display.drawCircle(display.width() / 2, display.height() / 2, 30, SSD1306_INVERSE);
    display.display();
    delay(tapDuration);
    digitalWrite(TAP_PIN, LOW);
    updateDisplay();
    if (i == (nTaps - 1)) break;
    delay(wait);
  }
  digitalWrite(KICKOFF_PIN, HIGH);
  delay(300);
  digitalWrite(KICKOFF_PIN, LOW);
}
 
void loop() {

  int reading = digitalRead(TAP_START_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }  
  else if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        blink();
      }
    }
  }

  lastButtonState = reading;

  if (curVal > prevVal) {
    Serial.println('up');
    unsigned long tm = millis();
    lastButtonState = HIGH;
    lastDebounceTime = tm + debounceDelay * 4;
    bpm += ((tm - lastChangeTime) > changeDelay) ? 1 : 5;
    lastChangeTime = tm;
    if (bpm > 240) bpm = 10;
    updateDisplay();
  } 
  else if (curVal < prevVal) {
    Serial.println('down');
    unsigned long tm = millis();
    lastButtonState = HIGH;
    lastDebounceTime = tm + debounceDelay * 4;
    bpm -= ((tm - lastChangeTime) > changeDelay) ? 1 : 5;
    lastChangeTime = tm;
    if (bpm < 10) bpm = 240;
    updateDisplay();
  }
  
  prevVal = curVal;
}
