#include "EEPROM.h"
#include "TimerOne.h"
#include <Wire.h>
#include "RTClib.h"
#include <TimeLib.h>
#include <DS1307RTC.h>

#define LIGHTSENSOR_PIN A6
#define BUTTON_PIN A7
#define SOUND_PIN A2
#define COLON_PIN A0
#define ALARM_PIN A1

bool isDimmer = false;
int MAX_BRIGHTNESS_VALUE = 400;
int MIN_BRIGHTNESS_VALUE = 20;
int SET_BRIGHTNESS_VALUE = MAX_BRIGHTNESS_VALUE;

bool ARM_BUTTONS = true;
bool ZZZ, SET, NEG, POS, PRESSED;
int BUTTON_ZZZ = 330;
int BUTTON_SET = 510;
int BUTTON_NEG = 710;
int BUTTON_POS = 850;
int BUTTON_OFF = 1024;

int buttonState = 1024;
int lightState = 1024;
bool playAudio = false;

RTC_DS1307 rtc;
tmElements_t tm;

static int digitPins[] = { 3, 9, 10, 11, 5, 6 };
static int segmentPins[] = { 12, 8, 7, 4, 2, 1, 0 };

const char *monthName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
String characters = "0123456789abcdefghijlnopqrstuyz- ";
byte chars[] = { 63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 119, 124, 57, 94, 121, 113, 61, 118, 4, 14, 56, 84, 92, 115, 103, 80, 109, 120, 28, 110, 91, 64, 0 };

byte displayText[8];
int digitNo;

void setDimmer() {
  isDimmer = !isDimmer;
  if (isDimmer) {
    MAX_BRIGHTNESS_VALUE = 250;
    MIN_BRIGHTNESS_VALUE = 4;
    SET_BRIGHTNESS_VALUE = MAX_BRIGHTNESS_VALUE;
  } else {
    MAX_BRIGHTNESS_VALUE = 400;
    MIN_BRIGHTNESS_VALUE = 20;
    SET_BRIGHTNESS_VALUE = MAX_BRIGHTNESS_VALUE;
  }
}

void Display() {
  digitNo = (digitNo + 1) % 6;
  DisplayDigit(digitNo, displayText[digitNo]);

  DisplayColon();

}

void DisplayColon() {
  digitalWrite(COLON_PIN, HIGH);
  delayMicroseconds(SET_BRIGHTNESS_VALUE/2);
  digitalWrite(COLON_PIN, LOW);
}

void DisplayDigit(int digit, byte segments) {

  int Brightness = (digit < 4) ? SET_BRIGHTNESS_VALUE : SET_BRIGHTNESS_VALUE/2;

  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], (segments >> i) & 1);
  }

  digitalWrite(digitPins[digit], HIGH);
  delayMicroseconds(Brightness);
  digitalWrite(digitPins[digit], LOW);
}

void UpdateDisplayNumber(byte digit, byte number) {
  displayText[digit] = chars[number];
}

void setup() {

  Timer1.initialize(1200);
  Timer1.attachInterrupt(Display);
  Timer1.start();

  for (int i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT);
    digitalWrite(segmentPins[i], LOW);
  }

  for (int i = 0; i < 6; i++) {
    pinMode(digitPins[i], OUTPUT);
    digitalWrite(digitPins[i], LOW);
  }

  pinMode(COLON_PIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  pinMode(SOUND_PIN, OUTPUT);
  digitalWrite(SOUND_PIN, LOW);

  pinMode(LIGHTSENSOR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  rtc.begin();
  if (!rtc.isrunning()) {
    tm.Hour = 12;
    tm.Minute = 0;
    tm.Second = 0;
    tm.Day = 1;
    tm.Month = 1;
    tm.Year = CalendarYrToTm(1970);
    // Initialize RTC
    RTC.write(tm);
  }

}

int settingTime = 0;
int hr, mn, sc;
void loop() {

  digitalWrite(ALARM_PIN, LOW);
  buttonState = analogRead(BUTTON_PIN);

  if ( (abs(buttonState - BUTTON_OFF) < 50) ) {
    ARM_BUTTONS = true;
  } else { 

    if (ARM_BUTTONS && (abs(buttonState - BUTTON_SET) < 50)) {
      ARM_BUTTONS = false;
      settingTime = (settingTime + 1) % 4;
      if (settingTime == 0) {
        rtc.adjust(DateTime(1970, 1, 1, hr, mn, sc));
      }
    }

    if (ARM_BUTTONS && (abs(buttonState - BUTTON_NEG) < 50)) {
      ARM_BUTTONS = false;
      switch (settingTime) {
        case 1: hr = (24+hr-1) % 24; break;
        case 2: mn = (60+mn-1) % 60; break;
        case 3: sc = (60+sc-1) % 60; break;
      }
    }

    if (ARM_BUTTONS && (abs(buttonState - BUTTON_POS) < 50)) {
      ARM_BUTTONS = false;
      switch (settingTime) {
        case 1: hr = (hr+1) % 24; break;
        case 2: mn = (mn+1) % 60; break;
        case 3: sc = (sc+1) % 60; break;
      }
    }

    if (ARM_BUTTONS && (abs(buttonState - BUTTON_ZZZ) < 50)) {
      ARM_BUTTONS = false;
      setDimmer();
    }
  } 

  // lightState is around 250 in the light and around 750 in the dark
  lightState = constrain( analogRead(LIGHTSENSOR_PIN), 250, 750 );
  SET_BRIGHTNESS_VALUE = map(lightState, 250, 750, MAX_BRIGHTNESS_VALUE, MIN_BRIGHTNESS_VALUE); 

  if (settingTime == 0) { 
    DateTime now = rtc.now();
    hr = now.hour();
    mn = now.minute();
    sc = now.second();
    digitalWrite(ALARM_PIN, LOW);
  } else {
    digitalWrite(ALARM_PIN, HIGH);
  }

  UpdateDisplayNumber(0, hr / 10);
  UpdateDisplayNumber(1, hr % 10);
  UpdateDisplayNumber(2, mn / 10);
  UpdateDisplayNumber(3, mn % 10);
  UpdateDisplayNumber(4, sc / 10);
  UpdateDisplayNumber(5, sc % 10);
}