#include <RTClib.h> //For RTC
#include<Wire.h>    //Also for RTC
#include <LiquidCrystal.h> //For LCD: https://www.arduino.cc/reference/en/libraries/liquidcrystal/
#include <Stepper.h> //For stepper motor: https://www.arduino.cc/reference/en/libraries/stepper/



const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

RTC_DS1307 rtc;

const int stepsPerRevolution = 2038;
Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);

//Function Def
void updateTime();

void setup(){
    //Serial.begin(9600);
    lcd.begin(16, 2);
    
    Wire.begin();
    rtc.begin();
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop(){
    updateTime();
}

void updateTime(){
    DateTime now = rtc.now();
    lcd.setCursor(0, 0);
    lcd.print("Date: ");
    lcd.print(now.day(), DEC);
    lcd.print('/');
    lcd.print(now.month(), DEC);
    lcd.print('/');
    lcd.print(now.year(), DEC);

    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    lcd.print(now.minute(), DEC);
    lcd.print(':');
    lcd.print((now.second() /10) % 10);
    lcd.print(now.second() % 10, DEC);
}