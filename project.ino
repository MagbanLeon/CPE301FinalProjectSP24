// Names: Jenna Chang, Sean Crum, Leon Magbanua, Anthony Martinez
// Class: CPE 301-1001
// Date: 1 May 2024

// USABLE LIBRARIES
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <DHT.h>
#include <RTClib.h>

// UART0
#define RDA 0x80
#define TBE 0x20
volatile unsigned char *myUCSR0A = (unsigned char *) 0xC0;
volatile unsigned char *myUCSR0B = (unsigned char *) 0xC1;
volatile unsigned char *myUCSR0C = (unsigned char *) 0xC2;
volatile unsigned int  *myUBRR0  = (unsigned int  *) 0xC4;
volatile unsigned char *myUDR0   = (unsigned char *) 0xC6;

// LCD
const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal ourLCD(RS, EN, D4, D5, D6, D7);

// WATER SENSOR
#define WATER_SENSOR A0
#define WATER_LIMIT 100

// STEPPER
#define STEPPER_PIN1 40
#define STEPPER_PIN2 41
#define STEPPER_PIN3 42
#define STEPPER_PIN4 43
Stepper ourStepper(60, STEPPER_PIN1, STEPPER_PIN3, STEPPER_PIN2, STEPPER_PIN4);

// DHT SENSOR
#define HUMIDITY_LIMIT 50
DHT ourDHT(8, DHT11);

// FAN
#define RELAY_PIN 7

// BUTTONS
#define BUTTON_RIGHT 9
#define BUTTON_LEFT 10
#define BUTTON_ON_OFF 6
#define BUTTON_RESET 26

// LEDS
#define LED_PINR 32
#define LED_PINY 30
#define LED_PING 33
#define LED_PINB 31

// TIMER
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned char *myTIFR1  = (unsigned char *) 0x36;
volatile unsigned int  *myTCNT1  = (unsigned int  *) 0x84;
RTC_DS3231 rtc;
unsigned long time_now = 0;

// ADC
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

// Port A
volatile unsigned char* port_a = (unsigned char*) 0x22; 
volatile unsigned char* ddr_a  = (unsigned char*) 0x21; 
volatile unsigned char* pin_a  = (unsigned char*) 0x20; 
// Port B
volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24; 
volatile unsigned char* pin_b  = (unsigned char*) 0x23;
// Port C
volatile unsigned char* port_c = (unsigned char*) 0x28; 
volatile unsigned char* ddr_c  = (unsigned char*) 0x27; 
volatile unsigned char* pin_c  = (unsigned char*) 0x26; 
// Port D
volatile unsigned char* port_d = (unsigned char*) 0x2B; 
volatile unsigned char* ddr_d  = (unsigned char*) 0x2A; 
volatile unsigned char* pin_d  = (unsigned char*) 0x29;
// Port E
volatile unsigned char* port_e = (unsigned char*) 0x2E;
volatile unsigned char* ddr_e  = (unsigned char*) 0x2D;
volatile unsigned char* pin_e  = (unsigned char*) 0x2C;
// Port F
volatile unsigned char* port_f = (unsigned char*) 0x31; 
volatile unsigned char* ddr_f  = (unsigned char*) 0x30; 
volatile unsigned char* pin_f  = (unsigned char*) 0x2F;
// Port H
volatile unsigned char* port_h = (unsigned char*) 0x102; 
volatile unsigned char* ddr_h  = (unsigned char*) 0x101; 
volatile unsigned char* pin_h  = (unsigned char*) 0x100; 

enum SystemState{
  DISABLED,
  IDLE,
  ERROR_STATE,
  RUNNING,
  LEFT_STATE,
  RIGHT_STATE
};
SystemState currentState = DISABLED;
SystemState previousState = DISABLED;
bool systemEnabled = false;

void setup() {
  // Setup UART0
  U0Init(9600);

  // Setup TIMER
  setup_timer_regs();
  rtc.begin();
  DateTime now = DateTime(2024, 4, 23, 0, 0, 0);
  rtc.adjust(now);

  // Setup LCD
  ourLCD.begin(16, 2);
  ourLCD.setCursor(0, 0);

  // Setup WATER SENSOR
  *ddr_f &= ~(0x01 << 0); //pinMode(WATER_SENSOR, INPUT);

  // Setup STEPPER
  ourStepper.setSpeed(60);  // Set stepper motor speed to 60 RPM

  // Setup DHT SENSOR
  ourDHT.begin(); // Initialize DHT sensor

  // Setup FAN
  *ddr_h |= (0x01 << 4); //pinMode(RELAY_PIN, OUTPUT);
  *port_h &= ~(0x01 << 4); //digitalWrite(RELAY_PIN, LOW);

  // Setup BUTTONS & LEDS
  *ddr_d &= ~(0x01 << 3); //pinMode(BUTTON_ON_OFF, INPUT);
  *ddr_a &= ~(0x01 << 4); //pinMode(BUTTON_RESET, INPUT);
  *ddr_h &= ~(0x01 << 6); //pinMode(BUTTON_LEFT, INPUT);
  *ddr_b &= ~(0x01 << 4); //pinMode(BUTTON_RIGHT, INPUT);
  *ddr_c |= (0x01 << 7); //pinMode(YELLOW, OUTPUT);
  *ddr_c |= (0x01 << 6); //pinMode(BLUE, OUTPUT);
  *ddr_c |= (0x01 << 5); //pinMode(RED, OUTPUT);
  *ddr_c |= (0x01 << 4); //pinMode(GREEN, OUTPUT);

  // setup ADC
  adc_init();

  // Setup Interupt
  attachInterrupt(digitalPinToInterrupt(18), toggleSystem, HIGH);
}

void loop () {
  // Sample data from water, temperature, humidity
  unsigned int water = adc_read(0);
  float temperature = ourDHT.readTemperature();
  float humidity = ourDHT.readHumidity();

  // Loop while the program is running
  switch(currentState){
    case DISABLED:
      disabledState();
      break;
    case IDLE:
      idledState(temperature, humidity, water);
      break;
    case ERROR_STATE:
      errorState(temperature, humidity, water);
      break;
    case RUNNING:
      runningState(temperature, humidity, water);
      break;
  }

  static unsigned long lastResetTime = 0;
  if(millis() - lastResetTime > 10000){
    ourLCD.begin(16,2);
    lastResetTime = millis();
  }
}

// UART0 FUNCTIONS
void U0Init(int U0baud){
 *myUCSR0A = 0x20, *myUCSR0B = 0x18, *myUCSR0C = 0x06;
 *myUBRR0  = (16000000 / 16 / U0baud - 1);
}
unsigned char kbhit(){
  return *myUCSR0A & RDA;
}
unsigned char getChar(){
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata){
  while((*myUCSR0A & TBE) == 0);
  *myUDR0 = U0pdata;
}

// ADC FUNCTIONS
void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

// STEPPER FUNCTION
void turnStepper(){
  if(!(*pin_h & (0x01 << 6))){
    if(previousState != LEFT_STATE){
      displayTime();
    }
    previousState = LEFT_STATE;
    ourStepper.step(60);
  }
  if(!(*pin_b & (0x01 << 4))){
    if(previousState != RIGHT_STATE){
      displayTime();
    }
    previousState = RIGHT_STATE;
    ourStepper.step(-60); 
  }
}

// TIMER FUNCTIONS
void setup_timer_regs(){
  *myTCCR1A= 0x00, *myTCCR1B= 0X00, *myTCCR1C= 0x00;  // setup the timer control registers
  *myTIFR1 |= 0x01, *myTIMSK1 |= 0x01;  // reset TOV flag and enable TOV interrupt
}
void displayTime(){
  DateTime now = rtc.now();
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();
  char numbers[10] = {'0','1','2','3','4','5','6','7','8','9'};
  int onesYear = year % 10;
  int tensYear = year / 10 % 10;
  int onesMonth = month % 10;
  int tensMonth = month / 10 % 10;
  int onesDay = day % 10;
  int tensDay = day / 10 % 10;
  int onesHour = hour % 10;
  int tensHour = hour / 10 % 10;
  int onesMinute = minute % 10;
  int tensMinute = minute / 10 % 10;
  int onesSecond = second % 10;
  int tensSecond = second / 10 % 10;
  
  U0putchar('M');
  U0putchar(':');
  U0putchar('D');
  U0putchar(':');
  U0putchar('Y');

  U0putchar(' ');
  
  U0putchar('H');
  U0putchar(':');
  U0putchar('M');
  U0putchar(':');
  U0putchar('S');

  U0putchar(' ');

  U0putchar(numbers[tensMonth]);
  U0putchar(numbers[onesMonth]);
  U0putchar(':');
  U0putchar(numbers[tensDay]);
  U0putchar(numbers[onesDay]);
  U0putchar(':');
  U0putchar(numbers[tensYear]);
  U0putchar(numbers[onesYear]);
  
  U0putchar(' ');

  U0putchar(numbers[tensHour]);
  U0putchar(numbers[onesHour]);
  U0putchar(':');
  U0putchar(numbers[tensMinute]);
  U0putchar(numbers[onesMinute]);
  U0putchar(':');
  U0putchar(numbers[tensSecond]);
  U0putchar(numbers[onesSecond]);

  U0putchar('\n');
  time_now = millis();
  while(millis() < time_now + 500); // delay(500);
}

// STATE FUNCTIONS
void disabledState(){
  turnStepper();
  *port_c |= (0x01 << 7); // YELLOW LED HIGH
  *port_c &= ~(0x01 << 6); // BLUE LED LOW
  *port_c &= ~(0x01 << 5); // RED LED LOW
  *port_c &= ~(0x01 << 4); // GREEN LED LOW
  *port_h &= ~(0x01 << 4); // FAN OFF
  ourLCD.clear();
  if(previousState == IDLE || previousState == RUNNING || previousState == ERROR_STATE){
    displayTime();
    previousState = DISABLED;
  }
  if(systemEnabled == true){
    currentState = IDLE;
    previousState = DISABLED;
  }
  else{
    currentState = DISABLED;
  }
}
void idledState(float temperature, float humidity, int water){
  turnStepper();
  *port_c &= ~(0x01 << 7); // YELLOW LED LOW
  *port_c &= ~(0x01 << 6); // BLUE LED LOW
  *port_c &= ~(0x01 << 5); // RED LED LOW
  *port_c |= (0x01 << 4); // GREEN LED HIGH

  time_now = millis();
  while(millis() < time_now + 500);

  printStats(temperature, humidity);

  if(previousState == DISABLED || previousState == RUNNING || previousState == ERROR_STATE){
    displayTime();
    previousState = IDLE;
  }
  if(water < WATER_LIMIT){
    currentState = ERROR_STATE;
  }
  else if(systemEnabled == false){
    currentState = DISABLED;
    previousState = IDLE;
  }
  else if(humidity > HUMIDITY_LIMIT){
    currentState = RUNNING;
  }
  else{
    currentState = IDLE;
  }
}

void errorState(float temperature, float humidity, int water){
  *port_h &= ~(0x01 << 4); // FAN OFF

  if(previousState == DISABLED || previousState == RUNNING || previousState == IDLE){
    displayTime();
    previousState = ERROR_STATE;
  }
  if(!(*pin_a & (0x01 << 4)) && water >= WATER_LIMIT){
    currentState = IDLE;
    previousState = ERROR_STATE;
  }
  if(systemEnabled == false){
    currentState = DISABLED;
    previousState = ERROR_STATE;
  }
  else{
    ourLCD.clear();
    ourLCD.setCursor(0, 0);
    ourLCD.print("Water level is");
    ourLCD.setCursor(0, 1);
    ourLCD.print("too low");

    *port_c &= ~(0x01 << 7); // YELLOW LED LOW
    *port_c &= ~(0x01 << 6); // BLUE LED LOW
    *port_c |= (0x01 << 5); // RED LED HIGH
    *port_c &= ~(0x01 << 4); // GREEN LED LOW

    time_now = millis();
    while(millis() < time_now + 500);
  }
}

void runningState(float temperature, float humidity, int water){
  turnStepper();
  if(previousState == DISABLED || previousState == IDLE || previousState == ERROR_STATE){
    displayTime();
    previousState = RUNNING;
  }
  *port_c &= ~(0x01 << 7); // YELLOW LED LOW
  *port_c |= (0x01 << 6); // BLUE LED HIGH
  *port_c &= ~(0x01 << 5); // RED LED LOW
  *port_c &= ~(0x01 << 4); // GREEN LED LOW

  printStats(temperature, humidity);

  time_now = millis();
  while(millis() < time_now + 500);
  if(humidity < HUMIDITY_LIMIT){
    *port_h &= ~(0x01 << 4); // FAN LOW
    currentState = IDLE;
    previousState = RUNNING;
  }
  else{
    *port_h |= (0x01 << 4); // FAN HIGH
  }
  if(water < WATER_LIMIT){
    currentState = ERROR_STATE;
    previousState = RUNNING;
  }
  if(systemEnabled == false){
    currentState = DISABLED;
    previousState = RUNNING;
  }
}

// INTERUPT FUNCTION
void toggleSystem(){
  if(*pin_d & (0x01 << 3)){
    systemEnabled = !systemEnabled;
  }
}

void printStats(float temperature, float humidity){
  ourLCD.clear();
  ourLCD.setCursor(0, 0);
  ourLCD.print("Temp: ");
  ourLCD.print(temperature);
  ourLCD.setCursor(0, 1);
  ourLCD.print("Humidity: ");
  ourLCD.print(humidity);
}
