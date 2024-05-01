//#include <LiquidCrystal.h> //For LCD: https://www.arduino.cc/reference/en/libraries/liquidcrystal/
//#include <Stepper.h> //For stepper motor: https://www.arduino.cc/reference/en/libraries/stepper/
//#include <RTClib.h> //For Real-Time Clock

void updateTime();

void setup(){
    lcd.begin(16, 2);
    

    Wire.begin();
    rtc.begin();
    rtc.adjust(DateTime(F(__DATE__)), (F(__TIME__)));
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