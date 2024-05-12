// Names: Jenna Chang, Sean Crum, Leon Magbanua, Anthony Martinez
// Class: CPE 301-1001
// Date: 1 May 2024

// Useable libraries
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <DHT.h>
#include <RTClib.h>

// UART0 registers
#define RDA 0x80
#define TBE 0x20
// UART0 pointers
volatile unsigned char *myUCSR0A = (unsigned char *) 0xC0;
volatile unsigned char *myUCSR0B = (unsigned char *) 0xC1;
volatile unsigned char *myUCSR0C = (unsigned char *) 0xC2;
volatile unsigned int  *myUBRR0  = (unsigned int  *) 0xC4;
volatile unsigned char *myUDR0   = (unsigned char *) 0xC6;

// define pins for the stepper motor
#define STEPPER_PIN1 40
#define STEPPER_PIN2 41
#define STEPPER_PIN3 42
#define STEPPER_PIN4 43

/* Timer1 Pointers */
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned char *myTIFR1  = (unsigned char *) 0x36;
volatile unsigned int  *myTCNT1  = (unsigned int  *) 0x84;

// LCD pins <--> Arduino pins
const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

//Water pin and threshold
#define WATER_SENSOR A0
#define WATER_LIMIT 100 // the threshold for that sensor

// Stepper motor
Stepper stepper(60, STEPPER_PIN1, STEPPER_PIN3, STEPPER_PIN2, STEPPER_PIN4);

// Fan pin
#define RELAY_PIN 7  // Define the pin for the relay control

//ADC Pointers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

//Port A Register Pointers
volatile unsigned char* port_a = (unsigned char*) 0x22; 
volatile unsigned char* ddr_a  = (unsigned char*) 0x21; 
volatile unsigned char* pin_a  = (unsigned char*) 0x20; 

//Port B Register Pointers
volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24; 
volatile unsigned char* pin_b  = (unsigned char*) 0x23;

//Port C Register Pointers
volatile unsigned char* port_c = (unsigned char*) 0x28; 
volatile unsigned char* ddr_c  = (unsigned char*) 0x27; 
volatile unsigned char* pin_c  = (unsigned char*) 0x26; 

//Port D Register Pointers
volatile unsigned char* port_d = (unsigned char*) 0x2B; 
volatile unsigned char* ddr_d  = (unsigned char*) 0x2A; 
volatile unsigned char* pin_d  = (unsigned char*) 0x29;

//Port E Register Pointers
volatile unsigned char* port_e = (unsigned char*) 0x2E; 
volatile unsigned char* ddr_e  = (unsigned char*) 0x2D; 
volatile unsigned char* pin_e  = (unsigned char*) 0x2C; 

//Port F Register Pointers
volatile unsigned char* port_f = (unsigned char*) 0x31; 
volatile unsigned char* ddr_f  = (unsigned char*) 0x30; 
volatile unsigned char* pin_f  = (unsigned char*) 0x2F;

//Port H Register Pointers
volatile unsigned char* port_h = (unsigned char*) 0x102; 
volatile unsigned char* ddr_h  = (unsigned char*) 0x101; 
volatile unsigned char* pin_h  = (unsigned char*) 0x100; 

//States
enum SystemState {
  DISABLED,
  IDLE,
  ERROR_STATE,
  RUNNING,
  LEFT_STATE,
  RIGHT_STATE
};

//Track states
SystemState currentState = DISABLED;
SystemState previousState = DISABLED;
bool systemEnabled = false;

RTC_DS3231 rtc;  // Initialize the RTC object
unsigned long time_now = 0;

void setup() {
  U0Init(9600);
  setup_timer_regs();
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);

  //pinMode(WATER_SENSOR, INPUT);
  *ddr_f &= ~(0x01 << 0); 
  *ddr_h &= ~(0x01 << 6); //pinMode(BUTTON_LEFT, INPUT);
  *ddr_b &= ~(0x01 << 4); //pinMode(BUTTON_RIGHT, INPUT);

  stepper.setSpeed(60);  // Set stepper motor speed to 60 RPM

  // setup the ADC
  adc_init();
  rtc.begin();

  // set the time on the RTC module
  DateTime now = DateTime(2024, 4, 23, 0, 0, 0);
  rtc.adjust(now);
}

void loop () {
  unsigned int waterLevel = adc_read(0);
  Serial.println("ADC reading: ");
  Serial.println(waterLevel);
  displayTime();
  stepper.step(60);

  if(waterLevel > WATER_LIMIT){
    lcd.clear();
    lcd.print("Water is wet.");
    delay(200);
  }
  else{
    lcd.clear();
    lcd.print("Water is dry.");
    delay(200);
  }
}

void U0Init(int U0baud) /* Serial.begin(int) */ {
 *myUCSR0A = 0x20, *myUCSR0B = 0x18, *myUCSR0C = 0x06;
 *myUBRR0  = (16000000 / 16 / U0baud - 1);
}
unsigned char kbhit() /* Serial.available() */ {
  return *myUCSR0A & RDA;
}
unsigned char getChar() /* Serial.read() */ {
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata) /* Serial.write(u_char) */ {
  while((*myUCSR0A & TBE) == 0);
  *myUDR0 = U0pdata;
}

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

void errorState(int waterLevel){ // float temperature, float humidity, 
  //digitalWrite(RELAY_PIN, LOW);
  *port_h &= ~(0x01 << 4);
  if(previousState == DISABLED || previousState == RUNNING || previousState == IDLE){
    displayTime();
    previousState = ERROR_STATE;
  }
  if(!(*pin_a & (0x01 << 4)) && waterLevel >= WATER_LIMIT){
    currentState = IDLE;
    previousState = ERROR_STATE;
  }
  if (systemEnabled == false) {
    currentState = DISABLED;
    previousState = ERROR_STATE;
  }
  else{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Water level is");
    lcd.setCursor(0, 1);
    lcd.print("too low");
    *port_c &= ~(0x01 << 7); //  digitalWrite(YELLOW, LOW);
    *port_c &= ~(0x01 << 6); //  digitalWrite(BLUE, LOW);
    *port_c |= (0x01 << 5); //  digitalWrite(RED, HIGH);
    *port_c &= ~(0x01 << 4); //  digitalWrite(GREEN, LOW);
    time_now = millis();
    while(millis() < time_now + 500); // delay(500);
  }
}

void turnState(){

  if(!(*pin_h & (0x01 << 6))){
    if(previousState != LEFT_STATE){
      displayTime();
    }
    previousState = LEFT_STATE;
  
    stepper.step(60); // Clockwise
  }
  if(!(*pin_b & (0x01 << 4))){
    if(previousState != RIGHT_STATE){
      displayTime();
    }
    previousState = RIGHT_STATE;
    stepper.step(-60); // Counterclockwise
  }
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

/* Timer1 Functions */
void setup_timer_regs() {
  *myTCCR1A= 0x00, *myTCCR1B= 0X00, *myTCCR1C= 0x00;  // setup the timer control registers
  *myTIFR1 |= 0x01, *myTIMSK1 |= 0x01;  // reset TOV flag and enable TOV interrupt
}
