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
#include <DFPlayerMini_Fast.h>
#include <SoftwareSerial.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define ROTARY_PIN_A 2
#define ROTARY_PIN_B 3
#define ROTARY_BUTTON_PIN 4
#define TAP_TEMPO_RELAY_PIN 5
#define START_DRUMS_RELAY_PIN 6
#define PEDAL_TIP_PIN 7
#define PEDAL_RING_PIN 8
#define NUM_COUNTS_PIN_1 9 
#define NUM_COUNTS_PIN_2 12
#define AUDIO_RX 10
#define AUDIO_TX 11
#define ACCENT_PIN_1 A0
#define ACCENT_PIN_2 A1
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
unsigned long debounceDelay = 150;   // the debounce time; increase if the output flickers

SoftwareSerial audioSerial(AUDIO_RX, AUDIO_TX); // RX, TX

int volume = 30;  // 0 - 30

DFPlayerMini_Fast audio;

void setup() {
  pinMode(TAP_TEMPO_RELAY_PIN, OUTPUT);
  pinMode(START_DRUMS_RELAY_PIN, OUTPUT);
  pinMode(ROTARY_PIN_A, INPUT_PULLUP);
  pinMode(ROTARY_PIN_B, INPUT_PULLUP);
  pinMode(ROTARY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PEDAL_TIP_PIN, INPUT_PULLUP);
  pinMode(PEDAL_RING_PIN, INPUT_PULLUP);
  pinMode(NUM_COUNTS_PIN_1, INPUT_PULLUP);
  pinMode(NUM_COUNTS_PIN_2, INPUT_PULLUP);
  pinMode(ACCENT_PIN_1, INPUT_PULLUP);
  pinMode(ACCENT_PIN_2, INPUT_PULLUP);
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Start");
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), rotaryCLK, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B), rotaryDT, CHANGE);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    for(;;); // Don't proceed, loop forever
  }
  Serial.println("Display ready");
  display.clearDisplay();
  display.display();
  updateDisplay();
  audioSerial.begin(9600);
  audio.begin(audioSerial, true);
  audio.volume(volume);
}

void rotaryCLK() {
  if (digitalRead(ROTARY_PIN_A)==LOW) {
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
  if (digitalRead(ROTARY_PIN_B)==LOW) {
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

int numTaps() {
  int pin_1 = digitalRead(NUM_COUNTS_PIN_1) == LOW;
  int pin_2 = digitalRead(NUM_COUNTS_PIN_2) == LOW;
  return (pin_1) ? 3 : (pin_2) ? 6 : 4;   
}

void blink() {
  Serial.println("Blink");
  int n = numTaps();
  int tapDuration = 20;
  // https://planetcalc.com/8110/ - 0.92 @ 50, 0.79 @ 130
  double fudge = -0.001625 * bpm + 1.00125;
  int wait = int(fudge * 1000. * 60. / bpm - tapDuration);
  for (int i = 0; i < n; i++) {
    audio.play(1);
    digitalWrite(TAP_TEMPO_RELAY_PIN, HIGH);
    display.drawCircle(display.width() / 2, display.height() / 2, 32, SSD1306_INVERSE);
    display.drawCircle(display.width() / 2, display.height() / 2, 30, SSD1306_INVERSE);
    display.display();
    delay(tapDuration);
    digitalWrite(TAP_TEMPO_RELAY_PIN, LOW);
    updateDisplay();
    if (i == (n-1)) break;
    delay(wait);
  }
  delay(wait - 10);
  digitalWrite(START_DRUMS_RELAY_PIN, HIGH);
  delay(200);
  digitalWrite(START_DRUMS_RELAY_PIN, LOW);
}
 
void loop() {

  //
  // Pedals
  //
  
  bool pedal_tip = digitalRead(PEDAL_TIP_PIN) == LOW;
  bool pedal_ring = digitalRead(PEDAL_RING_PIN) == LOW;

  // Start / stop pedal
  if (pedal_tip && !pedal_ring) {
    digitalWrite(START_DRUMS_RELAY_PIN, HIGH);
    delay(200);
    while (digitalRead(PEDAL_TIP_PIN) == LOW) delay(50);
    digitalWrite(START_DRUMS_RELAY_PIN, LOW);
    delay(100);
  } 

  // Kick pedal
  else if (!pedal_tip && pedal_ring) {
    audio.play(2);
    while (digitalRead(PEDAL_RING_PIN) == LOW) delay(50);
    delay(100);
  }

  // Accent pedal
  else if (pedal_tip && pedal_ring) {
    bool a1 = digitalRead(ACCENT_PIN_1) == LOW;
    bool a2 = digitalRead(ACCENT_PIN_2) == LOW;
    audio.play((a1) ? 3 : (a2) ? 4 : 5);
    while (digitalRead(PEDAL_TIP_PIN) == LOW) delay(50);
    delay(100);
  }

  //
  // Tempo knob
  //
  
  int reading = digitalRead(ROTARY_BUTTON_PIN);

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
    //Serial.println('up');
    unsigned long tm = millis();
    lastButtonState = HIGH;
    lastDebounceTime = tm + debounceDelay * 4;
    bpm += ((tm - lastChangeTime) > changeDelay) ? 1 : 5;
    lastChangeTime = tm;
    if (bpm > 240) bpm = 10;
    updateDisplay();
  } 
  else if (curVal < prevVal) {
    //Serial.println('down');
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
