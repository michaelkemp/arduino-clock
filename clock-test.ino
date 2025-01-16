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

int MAX_BRIGHTNESS_VALUE = 400;
int MIN_BRIGHTNESS_VALUE = 20;
int SET_BRIGHTNESS_VALUE = MAX_BRIGHTNESS_VALUE;

int buttonState = 1024;
int lightState = 1024;

RTC_DS1307 rtc;
tmElements_t PCTime, RTCTime;

static int digitPins[] = { 3, 9, 10, 11, 5, 6 };
static int segmentPins[] = { 12, 8, 7, 4, 2, 1, 0 };

const char *monthName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
String characters = "0123456789abcdefghijlnopqrstuyz- ";
byte chars[] = { 63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 119, 124, 57, 94, 121, 113, 61, 118, 4, 14, 56, 84, 92, 115, 103, 80, 109, 120, 28, 110, 91, 64, 0 };

byte displayText[8];
int digitNo;

bool getTime(const char *str) {
    int Hour, Min, Sec;
    
    if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) {
      return false;
    }

    PCTime.Hour = Hour;
    PCTime.Minute = Min;
    PCTime.Second = Sec;
    return true;
}

bool getDate(const char *str) {
    char Month[12];
    int Day, Year;
    uint8_t monthIndex = -1;
    
    if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) {
      return false;
    }

    for (int i = 0; i < 12; i++) {
      if (strcmp(Month, monthName[i]) == 0) {
        monthIndex = i+1;
      }
    }

    if (monthIndex < 0) {
      return false;
    }

    PCTime.Day = Day;
    PCTime.Month = monthIndex;
    PCTime.Year = CalendarYrToTm(Year);
    return true;
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

  //get time from PC if plugged in to Arduino
  if (getDate(__DATE__) && getTime(__TIME__)) {
        DateTime now = rtc.now();
        RTCTime.Hour = now.hour(); 
        RTCTime.Minute = now.minute(); 
        RTCTime.Second = now.second(); 
        RTCTime.Day = now.day();
        RTCTime.Month = now.month(); 
        RTCTime.Year = now.year() - 1970;

        time_t PCSecs = makeTime( PCTime );
        time_t RTCSecs = makeTime( RTCTime );

        if ( abs(PCSecs - RTCSecs) > 0) {
          RTC.write(PCTime);
          rtc.adjust(rtc.now() + TimeSpan(5)); // time seems about 5 seconds slow -- adjust this here
        }        
    }

}

void loop() {

  buttonState = analogRead(BUTTON_PIN);

  // lightState is around 250 in the light and around 750 in the dark
  lightState = constrain( analogRead(LIGHTSENSOR_PIN), 250, 750 );

  SET_BRIGHTNESS_VALUE = map(lightState, 250, 750, MAX_BRIGHTNESS_VALUE, MIN_BRIGHTNESS_VALUE); 

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