/*
demo code V3
============
Make sure all required libraries are installed!

Select the COM port and the bootloader version (old or regular) that matches
your Arduino Nano, and upload the code.

Use of the demo code
=====================
You need to press [SET} to enter the settings menu. After selecting a menu 
option with the [-] and [+] keys, you must press [SET] again to open it. 

A value is saved immeditely after selecting or changing it. No key needs to 
be pressed to save a new value. 

Key functions: 
[-] decreses value, moves back through settings menu
[+] increases value, moves forward through settings menu
[set] calls settings menu, selects and saves settings
[ZZ] snooze button; snoozes alarm for a pre-set time after it came off
[ZZ] pressing the snooze button briefly shows the date on the display

Menu functions:
[SET] you entered the settings menu
[ADJUST TIME] press [-] or [+] to decrease or increase the time
[AL 1] press [-] or [+] to adjust the 1st alarm time. Press [SET] to switch 
between the following options: off | Mo-Fr | Sa-Su | every day 
[AL 2] same as alarm 1
[AL 3] same as alarm 1
[DISPLAY DATE] press [SET] to turn the date and day-of-week display on or off. 
When turned on, the date will be displayed intermittend with the time.
[DAY FIRST] press [SET] to change the order of day and month on the display.
[DISPLAY SECONDS] press [SET] to turn the seconds on or off on the display.
[12HR FORMAT] press [SET] to turn 12-hours time format on or off.
[BRIGHTNESS] press [SET] to switch between the 5 available brightness levels.
[SNOOZE] press [SET] to change the snooze time between 5 and 30 minutes.
[CLEAR SETTINGS] settings are saved in the Atmega's EEPROM. This option clears
the EEPROM and removes all settings e.g. for alarms.
[RAW BRIGHTNESS] this is a debug feature for the light sensor and shows the 
analog reading on A6. For example, if you want to develop an auto-dim function.

The demo code does not have a function to adjust the date or day-of-week since 
these values are set automatically when uploading the code and does not 
require an adjustment.

The Arduino IDE reads the current time and date from your computer and sets the
RTC when uploading the code. The time on your Nano clock will be a few seconds 
behind. You could just ignore it, change the time on your computer before the 
upload, or develop code that lets you reset or change the seconds.

This is a demo code to show you possibilities and spark your own imagination.
This code is not sold, it is not part of the product that it was designed for,
and it does not claim to be perfect or free from bugs.

The following libraries are required:
- DS1307RTC
- EEPROM
- TimerOne
- Wire
- RTClib
- TimeLib

PLEASE FEEL FREE TO CONTRIBUTE TO THE DEVELOPMENT. CORRECTIONS AND
ADDITIONS ARE HIGHLY APPRECIATED. SEND YOUR COMMENTS OR CODE TO:
support@universal-solder.com 

Please support us by purchasing products from UNIVERSAL-SOLDER.ca store!

Copyright (c) 2024 UNIVERSAL-SOLDER Electronics Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */

#include "EEPROM.h"
#include "TimerOne.h"
#include <Wire.h>
#include "RTClib.h"
#include <TimeLib.h>
#include <DS1307RTC.h>



#define MAX_BRIGHTNESS_VALUE 10
#define MIN_BRIGHTNESS_VALUE 500


RTC_DS1307 rtc;
tmElements_t tm;

#define FIRMWARE_VERSION 'C'
#define LIGHTSENSOR_PIN A6
#define BUTTON_PIN A7
#define SOUND_PIN A2

static int digitPins[] = {3, 9, 10, 11, 5, 6};
static int segmentPins[] = {12, 8, 7, 4, 2, 1, 0};
static int colonPin = A0;
static int alarmPin = A1;

const char *monthName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
String characters = "0123456789abcdefghijlnopqrstuyz- ";
byte chars[] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 119, 124, 57, 94, 121, 113, 61, 118, 4, 14, 56, 84, 92, 115, 103, 80, 109, 120, 28, 110, 91, 64, 0};
String daysofweek[] = {"su", "no", "tu", "ue", "th", "fr", "sa"};

byte displayText[8];
int brightness = 100;
float brightnessFloat = 100;

bool use12hr = false;
bool useSeconds = false;
bool useDisplayDate = true;
bool monthFirst = false;
int brightnessSetting = 0;
int snoozeAmount = 5;
int alarm1Type = 0;
int alarm2Type = 0;
int alarm3Type = 0;
int alarm1Time = 0;
int alarm2Time = 0;
int alarm3Time = 0;


int snoozeTime = 0;
bool alarm1Triggered = false;
bool alarm2Triggered = false;
bool alarm3Triggered = false;


int menuSelection = 0;
unsigned long startDisplayStringTime;


bool insideMenu = false;
unsigned long lastPress;
unsigned long startSnoozePress;
unsigned long lastLoopTime;
bool plusPressed = false;
bool minusPressed = false;
bool menuPressed = false;
bool snoozePressed = false;

int prevSec;
int dotsLoops;

int displayDateTemporarilyFor = 0;



int getBrightness() // OPTIONAL: brightness sensor reading
{
    float rawbr = ((analogRead(LIGHTSENSOR_PIN) - MAX_BRIGHTNESS_VALUE)*(1000 / MIN_BRIGHTNESS_VALUE) + 10) - 1;
    return constrain((1024.0 / rawbr) * 10.0, 1, 1000);
}


bool getTime(const char *str)
{
    int Hour, Min, Sec;
    if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
    tm.Hour = Hour;
    tm.Minute = Min;
    tm.Second = Sec;
    return true;
}


bool getDate(const char *str)
{
    char Month[12];
    int Day, Year;
    uint8_t monthIndex;
    if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
    for (monthIndex = 0; monthIndex < 12; monthIndex++)
        if (strcmp(Month, monthName[monthIndex]) == 0) break;
    if (monthIndex >= 12) return false;
    tm.Day = Day;
    tm.Month = monthIndex + 1;
    tm.Year = CalendarYrToTm(Year);
    return true;
}


void EepromWrite(int address, int number)
{ 
    byte byte1 = number >> 8;
    byte byte2 = number & 0xFF;
    EEPROM.write(address, byte1);
    EEPROM.write(address + 1, byte2);
}


int EepromRead(int address)
{
    byte byte1 = EEPROM.read(address);
    byte byte2 = EEPROM.read(address + 1);
    return (byte1 << 8) + byte2;
}


void SaveEEPROM()
{
    EepromWrite(0, use12hr);
    EepromWrite(2, useSeconds);
    EepromWrite(4, useDisplayDate);
    EepromWrite(6, monthFirst);
    EepromWrite(8, snoozeAmount);
    EepromWrite(10, brightnessSetting);
    
    EepromWrite(12, alarm1Type);
    EepromWrite(14, alarm2Type);
    EepromWrite(16, alarm3Type);
    EepromWrite(18, alarm1Time);
    EepromWrite(20, alarm2Time);
    EepromWrite(22, alarm3Time);
}


void LoadEEPROM()
{
    use12hr = EepromRead(0);
    useSeconds = EepromRead(2);
    useDisplayDate = EepromRead(4);
    monthFirst = EepromRead(6);
    snoozeAmount = EepromRead(8);
    brightnessSetting = EepromRead(10);
    
    alarm1Type = EepromRead(12);
    alarm2Type = EepromRead(14);
    alarm3Type = EepromRead(16);
    alarm1Time = EepromRead(18);
    alarm2Time = EepromRead(20);
    alarm3Time = EepromRead(22);
}


void DisplayString(String text, int scrollSpeed = 450, bool usemillis = false)
{
    if(scrollSpeed > 0)
    {
        if(usemillis)
        {
            if(startDisplayStringTime == 0) startDisplayStringTime = millis();
            int i = (millis() - startDisplayStringTime) / scrollSpeed % text.length()-3;
            UpdateDisplayText(0, text[i]);
            UpdateDisplayText(1, text[i+1]);
            UpdateDisplayText(2, text[i+2]);
            UpdateDisplayText(3, text[i+3]);
        }
        else for (int i = 0; i < text.length()-3; i++)
        {
            UpdateDisplayText(0, text[i]);
            UpdateDisplayText(1, text[i+1]);
            UpdateDisplayText(2, text[i+2]);
            UpdateDisplayText(3, text[i+3]);
            delay(scrollSpeed);
        }
    }
    
    else
    {
        UpdateDisplayText(0, text[0]);
        UpdateDisplayText(1, text[1]);
        UpdateDisplayText(2, text[2]);
        UpdateDisplayText(3, text[3]);
        delay(500);
    }
}


void UpdateDisplayText(byte digit, byte character)
{
    int index = characters.indexOf(character);
    if(index < 0) index = 30;
    displayText[digit] = chars[index];
}


void UpdateDisplayNumber(byte digit, byte number)
{
    displayText[digit] = chars[number];
}

int digitNo;

void Display()
{
    digitNo ++;
    if(digitNo > 5) digitNo = 0;
    
    if(digitNo == 0)
    {
        digitalWrite(colonPin, displayText[6] > 0);
        digitalWrite(alarmPin, displayText[7] > 0);
    }
    
    DisplayDigit(digitNo, displayText[digitNo]);
    
    if(digitNo == 0)
    {
        digitalWrite(colonPin, 0);
        digitalWrite(alarmPin, 0);
    }
    
    
    // digitalWrite(colonPin, displayText[6] > 0);
    // digitalWrite(alarmPin, displayText[7] > 0);
    
    // DisplayDigit(0, displayText[0]);
    
    // digitalWrite(colonPin, 0);
    // digitalWrite(alarmPin, 0);
    
    // DisplayDigit(1, displayText[1]);
    // DisplayDigit(2, displayText[2]);
    // DisplayDigit(3, displayText[3]);
    // DisplayDigit(4, displayText[4]);
    // DisplayDigit(5, displayText[5]);
}


void DisplayDigit(int digit, byte segments)
{

    for (int i = 0; i < 7; i++) {
        digitalWrite(segmentPins[i], (segments >> i) & 1);
    }
    
    digitalWrite(digitPins[digit], HIGH);
    delayMicroseconds(brightness);
    digitalWrite(digitPins[digit], LOW);
}


void DisplayCurrentTime(int hour, int minute, int second = 0)
{
    
    bool isPM = false;
    if(use12hr)
    {
        if(hour >= 12)
        {
            hour -= 12;
            isPM = true;
        }
        if(hour == 0) hour = 12;
    }
    
    if(hour < 10) UpdateDisplayText(0, ' ');
    else UpdateDisplayNumber(0, hour / 10);
    UpdateDisplayNumber(1, hour % 10);
    UpdateDisplayNumber(2, minute / 10);
    UpdateDisplayNumber(3, minute % 10);
    if(useSeconds)
    {
        UpdateDisplayNumber(4, second / 10);
        UpdateDisplayNumber(5, second % 10);
        
    }
    else
    {
        if(use12hr)
        {
            UpdateDisplayText(4, isPM ? 'p' : 'a');
            UpdateDisplayText(5, 'n');
        }
        else
        {
            UpdateDisplayText(4, ' ');
            UpdateDisplayText(5, ' ');
        }
    }
}


void DisplayCurrentDate(int day, int month, int dayofweek)
{
    
    if(monthFirst)
    {
        if(month < 10) UpdateDisplayText(0, ' ');
        else UpdateDisplayNumber(0, month / 10);
        UpdateDisplayNumber(1, month % 10);
        if(day < 10) UpdateDisplayText(2, ' ');
        else UpdateDisplayNumber(2, day / 10);
        UpdateDisplayNumber(3, day % 10);
    }
    else
    {
        if(day < 10) UpdateDisplayText(0, ' ');
        else UpdateDisplayNumber(0, day / 10);
        UpdateDisplayNumber(1, day % 10);
        if(month < 10) UpdateDisplayText(2, ' ');
        else UpdateDisplayNumber(2, month / 10);
        UpdateDisplayNumber(3, month % 10);
    }
    
    UpdateDisplayText(4, daysofweek[dayofweek][0]);
    UpdateDisplayText(5, daysofweek[dayofweek][1]);
    displayText[6] = 0;
}


void alarmSelector(int alarmNumber)
{
    delay(200);
    int alarmType = 0;
    int hour = 0;
    int minute = 0;
    if(alarmNumber == 1)
    {
        alarmType = alarm1Type;
        hour = alarm1Time / 60;
        minute = alarm1Time % 60;
    }
    if(alarmNumber == 2)
    {
        alarmType = alarm2Type;
        hour = alarm2Time / 60;
        minute = alarm2Time % 60;
    }
    if(alarmNumber == 3)
    {
        alarmType = alarm3Type;
        hour = alarm3Time / 60;
        minute = alarm3Time % 60;
    }
    while(lastPress + 10000 > millis())
    {
        int buttonState = analogRead(BUTTON_PIN);
        snoozePressed = false;
        menuPressed = false;
        minusPressed = false;
        plusPressed = false;
        if(buttonState < 400) snoozePressed = true;
        else if(buttonState < 550) menuPressed = true;
        else if(buttonState < 780) minusPressed = true;
        else if(buttonState < 880) plusPressed = true;
        
        if(menuPressed)
        {
            lastPress = millis();
            alarmType ++;
            if(alarmType > 3) alarmType = 0;
            
            if(alarmType == 0) DisplayString("off ", 1000);
            if(alarmType == 1) DisplayString("  non-fri  ");
            if(alarmType == 2) DisplayString("  sat-sun  ");
            if(alarmType == 3) DisplayString("  eueryday  ");
        }
        
        if(plusPressed)
        {
            minute += 5;   
            lastPress = millis();
        }
        if(minusPressed)
        {
            minute -= 5;
            lastPress = millis();
        }
        if(minute < 0)
        {
            minute = 55;
            hour -= 1;
        }
        if(minute > 55)
        {
            minute = 0;
            hour += 1;
        }
        if(hour < 0) hour=23;
        if(hour > 23) hour = 0;
        
        DisplayCurrentTime(hour, minute);
        
        delay(100);
        
        if(alarmNumber == 1)
        {
            alarm1Type = alarmType;
            alarm1Time = hour * 60 + minute;
        }
        if(alarmNumber == 2)
        {
            alarm2Type = alarmType;
            alarm2Time = hour * 60 + minute;
        }
        if(alarmNumber == 3)
        {
            alarm3Type = alarmType;
            alarm3Time = hour * 60 + minute;
        }
    }
}


void DisplayMenu()
{
    
    menuSelection = constrain(menuSelection, 0, 12);
        
    switch (menuSelection)
    {
    case 0:
        DisplayString("set ", 0, true);
    break;
    
    case 1:
        DisplayString("adjust tine  ", 450, true);
        if(menuPressed)
        {
            while(lastPress + 10000 > millis())
            {
                int buttonState = analogRead(BUTTON_PIN);
                snoozePressed = false;
                menuPressed = false;
                minusPressed = false;
                plusPressed = false;
                if(buttonState < 400) snoozePressed = true;
                else if(buttonState < 550) menuPressed = true;
                else if(buttonState < 780) minusPressed = true;
                else if(buttonState < 880) plusPressed = true;
                
                DateTime now = rtc.now();
                int hour = now.hour();
                int minute = now.minute();
                int second = now.second();
                DisplayCurrentTime(hour, minute, second);
                if(plusPressed)
                {
                    DateTime now = rtc.now() + TimeSpan(0, 0, 1, 0);
                    rtc.adjust(now);
                    lastPress = millis();
                }
                if(minusPressed)
                {
                    DateTime now = rtc.now() - TimeSpan(0, 0, 1, 0);
                    rtc.adjust(now);
                    lastPress = millis();
                }
                delay(100);
            }
        }
    break;
    
    case 2:
        DisplayString("al 1", 0, true);
        if(menuPressed)
        {
            alarmSelector(1);
        }
    break;
    
    case 3:
        DisplayString("al 2", 0, true);
        if(menuPressed)
        {
            alarmSelector(2);
        }
    break;
    
    case 4:
        DisplayString("al 3", 0, true);
        if(menuPressed)
        {
            alarmSelector(3);
        }
    break;
    
    case 5:
        DisplayString("display date  ", 450, true);
        if(menuPressed)
        {
            if(useDisplayDate)
            {
                DisplayString("off ", 1000);
                useDisplayDate = false;
            }
            else
            {
                DisplayString("on  ", 1000);
                useDisplayDate = true;
            }
        }
    break;
    
    case 6:
        DisplayString("day first  ", 450, true);
        if(menuPressed)
        {
            if(monthFirst)
            {
                DisplayString("on ", 1000);
                monthFirst = false;
            }
            else
            {
                DisplayString("off  ", 1000);
                monthFirst = true;
            }
        }
    break;
    
    case 7:
        DisplayString("display seconds  ", 450, true);
        if(menuPressed)
        {
            if(useSeconds)
            {
                DisplayString("off ", 1000);
                useSeconds = false;
            }
            else
            {
                DisplayString("on  ", 1000);
                useSeconds = true;
            }
        }
    break;
     
    case 8:
        DisplayString("12hr fornat  ", 450, true);
        if(menuPressed)
        {
            if(use12hr)
            {
                DisplayString("off ", 1000);
                use12hr = false;
            }
            else
            {
                DisplayString("on  ", 1000);
                use12hr = true;
            }
        }
    break;
    
    case 9:
        DisplayString("brightness  ", 450, true);
        if(menuPressed)
        {
            brightnessSetting += 1;
            if(brightnessSetting > 5) brightnessSetting = 0;
            if(brightnessSetting < 1) brightnessSetting = 1;
            
            brightness = (brightnessSetting - 1) * (brightnessSetting) * 50;
            DisplayString(String(brightnessSetting) + "   ", 0);
            DisplayString(String(brightnessSetting) + "   ", 0);
        }
    break;
    
    case 10:
        DisplayString("snooze anount  ", 450, true);
        if(menuPressed)
        {
            snoozeAmount += 5;
            if(snoozeAmount > 30) snoozeAmount = 5;
            DisplayString(String(snoozeAmount) + " n  ", 0);
            DisplayString(String(snoozeAmount) + " n  ", 0);
        }
    break;
    
    case 11:
        DisplayString("clear settings  ", 450, true);
        if(menuPressed)
        {
            use12hr = false;
            useSeconds = false;
            useDisplayDate = true;
            monthFirst = false;
            snoozeAmount = 5;
            alarm1Type = 0;
            alarm2Type = 0;
            alarm3Type = 0;
            alarm1Time = 0;
            alarm2Time = 0;
            alarm3Time = 0;
            UpdateDisplayText(4, '-');
            UpdateDisplayText(5, '-');
            DisplayString("  clear  ");
        }
    break;
    case 12:
        DisplayString("rau brightness " + String(analogRead(LIGHTSENSOR_PIN)) + "  ", 450, true);
    break;
    
    }
    UpdateDisplayText(4, ' ');
    UpdateDisplayText(5, ' ');
    displayText[6] = 0;
}


void setup()
{
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
    
    brightness = getBrightness();
    brightnessFloat = brightness;
    
    DisplayString("    canaduino    ");
    
    
    if (! rtc.begin())
    {
        UpdateDisplayText(4, '-');
        UpdateDisplayText(5, '-');
        while(1) DisplayString("  error  uith  rtc  ", 500);
    }
    
    bool resetEEPROM = EEPROM.read(100) != FIRMWARE_VERSION;
    
    if (getDate(__DATE__) && getTime(__TIME__))
    {
        DateTime now = rtc.now();
        unsigned long nowtimestamp = (now.year() - 1970) * 32140800 + now.month() * 2678400 + now.day() * 86400 + now.hour() * 3600 + now.minute() * 60 + now.second();
        unsigned long tmtimestamp = tm.Year * 32140800 + tm.Month * 2678400 + tm.Day * 86400 + tm.Hour * 3600 + tm.Minute * 60 + tm.Second;
        if(tmtimestamp >= (nowtimestamp - 30) || resetEEPROM)
        {
            RTC.write(tm);
            rtc.adjust(rtc.now() + TimeSpan(10));
            DisplayString("  updating tine  ");
        }
    }
    
    if(resetEEPROM)
    {
        for (int i = 0; i < 100; i++)
        {
            EEPROM.write(i, 0);
        }
        EEPROM.write(100, FIRMWARE_VERSION);
        SaveEEPROM();
        DisplayString("  eepron reset  ");
        
    }
    
    LoadEEPROM();
    
}


void loop()
{
    brightness = (brightnessSetting - 1) * (brightnessSetting) * 50;
    brightness = constrain(brightness, 0, 1000);
    
    
    int buttonState = analogRead(BUTTON_PIN);
    plusPressed = false;
    minusPressed = false;
    menuPressed = false;
    snoozePressed = false;
    
    
    if(buttonState < 400) snoozePressed = true;
    else if(buttonState < 550) menuPressed = true;
    else if(buttonState < 780) minusPressed = true;
    else if(buttonState < 880) plusPressed = true;
    
    if(menuPressed && millis() - lastPress > 500)
    {
        lastPress = millis();
        if(!insideMenu) menuSelection = 0;
        insideMenu=true;
    }
    if(plusPressed && insideMenu && millis() - lastPress > 500)
    {
        lastPress = millis();
        menuSelection ++;
        startDisplayStringTime = 0;
    }
    if(minusPressed && insideMenu && millis() - lastPress > 500)
    {
        lastPress = millis();
        menuSelection --;
        startDisplayStringTime = 0;
    }
    if(snoozePressed && millis() - lastPress > 500)
    {
        lastPress = millis();
        if(startSnoozePress == 0) startSnoozePress = millis();
    }
    
    
    if(lastPress + 10000 < millis())
    {
        if(insideMenu) SaveEEPROM();
        insideMenu = false;
    }
    
    if(insideMenu)
    {
        DisplayMenu();
    }
    
    else
    {
        
        DateTime now = rtc.now();
        int hour = now.hour();
        int minute = now.minute();
        int second = now.second();
        int day = now.day();
        int month = now.month();
        int dayofweek = now.dayOfTheWeek();
        
        if(second != prevSec)
        {
            prevSec = second;
            dotsLoops = 0;
        }
        
        int nowOneInt = hour * 60 + minute;
        
        if(alarm1Type == 3 || (alarm1Type == 1 && (dayofweek != 6 && dayofweek != 0) ) || (alarm1Type == 2 && (dayofweek == 6 || dayofweek == 0)))
            if(nowOneInt == alarm1Time && snoozeTime >= 0) alarm1Triggered = true;
        
        if(alarm2Type == 3 || (alarm2Type == 1 && (dayofweek != 6 && dayofweek != 0) ) || (alarm2Type == 2 && (dayofweek == 6 || dayofweek == 0)))
            if(nowOneInt == alarm2Time && snoozeTime >= 0) alarm2Triggered = true;

        if(alarm3Type == 3 || (alarm3Type == 1 && (dayofweek != 6 && dayofweek != 0) ) || (alarm3Type == 2 && (dayofweek == 6 || dayofweek == 0)))
            if(nowOneInt == alarm3Time && snoozeTime >= 0) alarm3Triggered = true;
        
        
        bool alarm1Sound = alarm1Triggered && (nowOneInt >= (alarm1Time + snoozeTime)%1440);
        bool alarm2Sound = alarm2Triggered && (nowOneInt >= (alarm2Time + snoozeTime)%1440);
        bool alarm3Sound = alarm3Triggered && (nowOneInt >= (alarm3Time + snoozeTime)%1440);
        
        if(minute % 5 > 2 && snoozeTime < 0) snoozeTime = 0;
        
        if(alarm1Sound + alarm2Sound + alarm3Sound)
        {
            digitalWrite(SOUND_PIN, HIGH);
            if(startSnoozePress > 0 && !snoozePressed)
            {
                snoozeTime += snoozeAmount;
                digitalWrite(SOUND_PIN, LOW);
                displayText[6] = 0;
                UpdateDisplayText(4, ' ');
                UpdateDisplayText(5, ' ');
                DisplayString("  snooze  ");
            }
        }
        else
        {
            digitalWrite(SOUND_PIN, LOW);
            if(startSnoozePress > 0) displayDateTemporarilyFor = 20;
        }
        
        if(startSnoozePress > 0  && millis() - startSnoozePress > 2000 && snoozePressed)
        {
            alarm1Triggered = false;
            alarm2Triggered = false;
            alarm3Triggered = false;
            snoozeTime = -1;
            digitalWrite(SOUND_PIN, LOW);
            displayText[6] = 0;
            UpdateDisplayText(4, ' ');
            UpdateDisplayText(5, ' ');
            DisplayString("  alarn off  ");
        }
        
        if((useDisplayDate && millis()%30000 > 15000) || displayDateTemporarilyFor > 0) DisplayCurrentDate(day, month, dayofweek);
        else
        {
            displayText[6] = (dotsLoops < 2);
            DisplayCurrentTime(hour, minute, second);
        }
        
    }
    
    if(alarm1Type + alarm2Type + alarm3Type) displayText[7] = 1;
    else displayText[7] = 0;
    
    if(!snoozePressed) startSnoozePress = 0;
    
    // delay(150);
    // delay(150 - (millis() - lastLoopTime));
    while(millis() - lastLoopTime < 150)
    {
        delay(10);
    }
    lastLoopTime = millis();
    dotsLoops ++;
    if(displayDateTemporarilyFor > 0) displayDateTemporarilyFor --;
    
}
