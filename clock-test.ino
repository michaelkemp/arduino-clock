#include "EEPROM.h"
#include "TimerOne.h"
#include <Wire.h>
#include "RTClib.h"
#include <TimeLib.h>
#include <DS1307RTC.h>

#define FIRMWARE_VERSION 'C'
#define LIGHTSENSOR_PIN A6
#define BUTTON_PIN A7
#define SOUND_PIN A2

#define MAX_BRIGHTNESS_VALUE 500
#define MIN_BRIGHTNESS_VALUE 200

RTC_DS1307 rtc;
tmElements_t tm;

static int digitPins[] = { 3, 9, 10, 11, 5, 6 };
static int segmentPins[] = { 12, 8, 7, 4, 2, 1, 0 };
static int colonPin = A0;
static int alarmPin = A1;

String characters = "0123456789abcdefghijlnopqrstuyz- ";
byte chars[] = { 63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 119, 124, 57, 94, 121, 113, 61, 118, 4, 14, 56, 84, 92, 115, 103, 80, 109, 120, 28, 110, 91, 64, 0 };

byte displayText[8];
int digitNo;

void Display() {
  digitNo = (digitNo + 1) % 6;
  DisplayDigit(digitNo, displayText[digitNo]);
}

void DisplayDigit(int digit, byte segments) {
  int Brightness = MAX_BRIGHTNESS_VALUE;
  if (digit >= 4) {
      Brightness /= 2;
  }

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

  pinMode(colonPin, OUTPUT);
  pinMode(alarmPin, OUTPUT);
  pinMode(SOUND_PIN, OUTPUT);
  digitalWrite(SOUND_PIN, LOW);

  rtc.begin();
}

void loop() {

  DateTime now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();

  UpdateDisplayNumber(0, hour / 10);
  UpdateDisplayNumber(1, hour % 10);
  UpdateDisplayNumber(2, minute / 10);
  UpdateDisplayNumber(3, minute % 10);
  UpdateDisplayNumber(4, second / 10);
  UpdateDisplayNumber(5, second % 10);
}