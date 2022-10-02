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
#include <TimerOne.h>
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
#define AUDIO_RX 10
#define AUDIO_TX 11
#define NUM_COUNTS_PIN_2 12
#define ONBOARD_LED 13
#define TEMPO_LED_PIN_1 14 // A0
#define TEMPO_LED_PIN_2 15 // A1
#define TEMPO_LED_PIN_3 16 // A2
#define TEMPO_LED_PIN_4 17 // A3
#define ACCENT_PIN_1 A6
#define ACCENT_PIN_2 A7
#define IDLE_11 0
#define SCLK_01 1
#define SCLK_00 2
#define SCLK_10 3
#define SDT_10 4
#define SDT_00 5
#define SDT_01 6

int state = IDLE_11;
volatile int tempoSet = 0;
volatile int drumsStarted = 0;
volatile int doTaps = -1;
volatile int beat = 0;
volatile int pedalStart = 0;
volatile int autoStart = 1;
volatile int invertDisplay = 0;
volatile int repeatPlay = 0;
int bpm = 90;
int period = 200;
int curVal = 0;
int prevVal = 0;

unsigned long lastChangeTime = 0;
unsigned long changeDelay = 100;

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers

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
  pinMode(TEMPO_LED_PIN_1, OUTPUT);
  pinMode(TEMPO_LED_PIN_2, OUTPUT);
  pinMode(TEMPO_LED_PIN_3, OUTPUT);
  pinMode(TEMPO_LED_PIN_4, OUTPUT);

  // Initialize display.
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println("No display");
    while (1);// Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();
  updateDisplay();

  // Start metronome.
  Timer1.initialize(1000000 * 60 / bpm);
  Timer1.attachInterrupt(tapIt);
  
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Start");

  // Add rotary encoder interrupts.
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), rotaryCLK, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B), rotaryDT, CHANGE);

  // Initialize audio card.
  audioSerial.begin(9600);
  audio.begin(audioSerial, false);
  audio.volume(volume);
}

void rotaryCLK() {
  if (digitalRead(ROTARY_PIN_A) == LOW) {
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
  if (digitalRead(ROTARY_PIN_B) == LOW) {
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
  return (pin_2) ? 6 : (pin_1) ? 4 : 3;   
}

int whichLED() {
  int n = numTaps();
  if (n == 4) {
    if (beat == 3) return TEMPO_LED_PIN_4;
    if (beat == 2) return TEMPO_LED_PIN_3;
    if (beat == 1) return TEMPO_LED_PIN_2;
    return TEMPO_LED_PIN_1;
  }
  else if (n == 3) {
    if (beat == 2) return TEMPO_LED_PIN_3;
    if (beat == 1) return TEMPO_LED_PIN_2;
    return TEMPO_LED_PIN_1;
  }
  else if (n == 6) {
    if (beat == 5) return TEMPO_LED_PIN_2;
    if (beat == 4) return TEMPO_LED_PIN_3;
    if (beat == 3) return TEMPO_LED_PIN_4;
    if (beat == 2) return TEMPO_LED_PIN_3;
    if (beat == 1) return TEMPO_LED_PIN_2;
    return TEMPO_LED_PIN_1;
  }
}

int whichAccent() {
  bool a1 = analogRead(ACCENT_PIN_1) < 20;
  bool a2 = analogRead(ACCENT_PIN_2) < 20;
  return (a1 && a2) ? 5 : (a2) ? 4 : 3;
}

void tapIt() {
  int flipDrumsRelay = 0;

  // Determine the LED pin based on the beat number.
  int ledPin = whichLED();

  // Light up current beat LED.
  digitalWrite(ledPin, HIGH);

  // If we used the pedal to start, do it on beat 1.
  if (beat == 0 && !autoStart && pedalStart) {
    digitalWrite(START_DRUMS_RELAY_PIN, HIGH);
    drumsStarted = 1;
    pedalStart = 0;
    tempoSet = 0;
    autoStart = 1;
    flipDrumsRelay = 1;
  }
  
  // If we're at zero taps and auto-start is enabled, start the drums.
  else if (doTaps == 0 && autoStart) {
    digitalWrite(START_DRUMS_RELAY_PIN, HIGH);
    invertDisplay = 1;
    drumsStarted = 1;
    pedalStart = 0;
    flipDrumsRelay = 1;
  }
  
  // If we're still counting taps, trigger the tap relay and external tap tempo signal.
  else if (doTaps > 0) {
    digitalWrite(TAP_TEMPO_RELAY_PIN, HIGH);
  }

  // Light beat LED and play click if we're still in the count-in.
  if (repeatPlay) {
    Serial.print("Play "); Serial.print(repeatPlay); Serial.println("");
    audio.play(repeatPlay);
  }
  else if (doTaps > 0) {
    audio.play(1);
  }
  
  delay(10000);

  // If we activated the start-drums relay, deactivate it.
  if (flipDrumsRelay) {
    digitalWrite(START_DRUMS_RELAY_PIN, LOW);
    doTaps--;
  }
  
  // If we are sending taps, turn the tap relay off now.
  else if (doTaps > 0) {
    digitalWrite(TAP_TEMPO_RELAY_PIN, LOW);
    doTaps--;
  }

  // Update beat number.
  beat = (beat + 1) % numTaps();

  // Turn off current beat LED.
  digitalWrite(ledPin, LOW);
}

int digitalReadDebounce(int pin) {
  // Read a digital signal while attempting to debounce the switch.
  int next = 0;
  int out = digitalRead(pin);
  delay(5);
  while ((next = digitalRead(pin)) != out) {
    delay(5);
    out = next;
  }
  return out;
}
 
void loop() {
  // Flip screen back after teaching taps.
  if (invertDisplay) {
    invertDisplay = 0;
    display.fillScreen(SSD1306_INVERSE);
    display.display();
  }

  //
  // Pedals
  //
  
  // Try to debounce the switch (don't use digitalReadDebounce because we want to interlace the reads).
  bool pedalTip = digitalRead(PEDAL_TIP_PIN) == LOW;
  bool pedalRing = digitalRead(PEDAL_RING_PIN) == LOW;
  delay(5);
  pedalTip = pedalTip && digitalRead(PEDAL_TIP_PIN) == LOW;
  pedalRing = pedalRing && digitalRead(PEDAL_RING_PIN) == LOW;

  // Start / stop pedal
  if (pedalTip && !pedalRing) {
    Serial.println("Start / Stop");
    
    // If a tempo has been set and we hit this pedal momentarily, 
    // start the beat at the next LED blink. This should start the 
    // drums in sync with the blinking LED.
    if (tempoSet && !drumsStarted && !autoStart) {
        tempoSet = 0;
        pedalStart = 1;
        delay(500);
    } 
    else {
      digitalWrite(START_DRUMS_RELAY_PIN, HIGH);
      delay(200);
      // Hold until pedal is released.
      while (digitalReadDebounce(PEDAL_TIP_PIN) == LOW) {
        delay(50);
      }
      digitalWrite(START_DRUMS_RELAY_PIN, LOW);
      delay(100);
    }
  } 

  // Kick pedal
  else if (!pedalTip && pedalRing) {
    Serial.println("Play kick");
    audio.play(2);
    delay(100);
    // Hold until pedal is released.
    while (digitalReadDebounce(PEDAL_RING_PIN) == LOW) {
      delay(50);
      repeatPlay = 2;
    }
    repeatPlay = 0;
    delay(50);
  }

  // Accent pedal
  else if (pedalTip && pedalRing) {
    int track = whichAccent();
    Serial.print("Play accent "); Serial.println(track);
    audio.play(track);
    delay(100);
    // Hold until pedal is released.
    while (digitalReadDebounce(PEDAL_TIP_PIN) == LOW) {
      delay(50);
      repeatPlay = track;
    }
    repeatPlay = 0;
    delay(50);
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
      // If a second press is detected during the taps, disable autoStart.
      if (doTaps >= 0 && buttonState == LOW) {
        autoStart = 0;
        invertDisplay = 1;
      }
      // If a press is detected, start the taps.
      else if (buttonState == LOW) {
        tempoSet = 1;
        pedalStart = 0;
        beat = 0;
        autoStart = 1;
        drumsStarted = 0;
        doTaps = numTaps();
        display.fillScreen(SSD1306_INVERSE);
        display.display();
        Timer1.restart();
        delay(100);
      }
    }
  }

  lastButtonState = reading;

  if (doTaps == -1 && curVal > prevVal) {
    //Serial.println('up');
    unsigned long tm = millis();
    lastButtonState = HIGH;
    lastDebounceTime = tm + debounceDelay * 4;
    bpm += ((tm - lastChangeTime) > changeDelay) ? 1 : 5;
    lastChangeTime = tm;
    if (bpm > 240) bpm = 10;
    updateDisplay();
    pedalStart = 0;
    tempoSet = 0;
    Timer1.setPeriod(1000000 * 60 / bpm);
  } 
  else if (doTaps == -1 && curVal < prevVal) {
    //Serial.println('down');
    unsigned long tm = millis();
    lastButtonState = HIGH;
    lastDebounceTime = tm + debounceDelay * 4;
    bpm -= ((tm - lastChangeTime) > changeDelay) ? 1 : 5;
    lastChangeTime = tm;
    if (bpm < 10) bpm = 240;
    updateDisplay();
    pedalStart = 0;
    tempoSet = 0;
    Timer1.setPeriod(1000000 * 60 / bpm);
  }
  
  prevVal = curVal;
}
