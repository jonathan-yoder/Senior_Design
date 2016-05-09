/* The code below is designed to test that the Arduino is
 * capable of reading the RTC module and determining if
 * a specified time has been reached.
 */ 

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Time.h>
#include <DS1307RTC.h>

#define WAKE_UP_TIME 15
#define BASE_HR_1 8
#define BASE_HR_2 17
#define BASE_MIN_1 0
#define BASE_MIN_2 0

#define interruptPinTrans 2
#define sleepPin 9

#define CLK_1_S 6
#define CLK_2_S 7
#define CLK_4_S 8
#define CLK_8_S 9

const int chipSelect = 10;

tmElements_t tm;
volatile int f_wdt=1;
File dataFile;
byte buf[60];
unsigned long timeToTransmit1, timeToTransmit2, currentTime;

void setup() {
  int hrToTransmit1, minToTransmit1, hrToTransmit2, minToTransmit2;
  Serial.begin(9600);
  int unit_id = 2;  // Get from EEPROM
  int off_hr = (int)(unit_id/4);
  int off_min = 15 * (unit_id % 4);
  Serial.print("offset hr: ");  Serial.println(off_hr);
  Serial.print("offset min: ");  Serial.println(off_min);
  minToTransmit1 = (BASE_MIN_1 + off_min) % 60;
  Serial.print("minTT1: ");  Serial.println(minToTransmit1);
  minToTransmit2 = (BASE_MIN_2 + off_min) % 60;
  Serial.print("minTT2: ");  Serial.println(minToTransmit2);
  hrToTransmit1 = (BASE_HR_1 + off_hr) % 24;
  hrToTransmit2 = (BASE_HR_2 + off_hr) % 24;
  if ((BASE_MIN_1 + off_min) / 60 >= 1){
    hrToTransmit1 = (hrToTransmit1 + 1) % 24;
  }
  if ((BASE_MIN_2 + off_min) / 60 >= 1){
    hrToTransmit2 = (hrToTransmit2 + 1) % 24;
  }
  Serial.print("hrTT1: ");  Serial.println(hrToTransmit1);
  Serial.print("hrTT2: ");  Serial.println(hrToTransmit2);
  
  timeToTransmit1 = (unsigned long)3600 * (unsigned long)hrToTransmit1
                    + (unsigned long) 60 * (unsigned long)minToTransmit1;
  timeToTransmit2 = (unsigned long)3600 * (unsigned long)hrToTransmit2
                    + (unsigned long) 60 * (unsigned long)minToTransmit2;
                    
  Serial.print("TTT1: ");  Serial.println(timeToTransmit1);
  Serial.print("TTT2: ");  Serial.println(timeToTransmit2);
  
  timeToTransmit1 = 67685;
                    
  Serial.print("set_TTT1: ");  Serial.println(timeToTransmit1);
  Serial.print("set_TTT2: ");  Serial.println(timeToTransmit2);
  pinMode(sleepPin, OUTPUT);

  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");

  bool parse=false;
  bool config=false;

  // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }

  if (parse && config) {
    Serial.print("DS1307 configured Time=");
    Serial.print(__TIME__);
    Serial.print(", Date=");
    Serial.println(__DATE__);
  } else if (parse) {
    Serial.println("DS1307 Communication Error :-{");
    Serial.println("Please check your circuitry");
  } else {
    Serial.print("Could not parse info from the compiler, Time=\"");
    Serial.print(__TIME__);
    Serial.print("\", Date=\"");
    Serial.print(__DATE__);
    Serial.println("\"");
  }

  sleep();  
}

void loop() {
  if (RTC.read(tm)) {
    Serial.print("Ok, Time = ");
    currentTime = (unsigned long)3600 * (unsigned long)tm.Hour
                   + (unsigned long)60 * (unsigned long)tm.Minute
                   + (unsigned long)tm.Second;
    Serial.print("Current: ");
    Serial.println(currentTime);
    if (currentTime - timeToTransmit1 <= 5 ||
        timeToTransmit1 - currentTime <= 5) {
      Serial.println("Time to transmit 1");
    }
    if (currentTime - timeToTransmit2 <= 5 ||
        timeToTransmit2 - currentTime <= 5) {
      Serial.println("Time to transmit 2");
    }
         
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
    }
    delay(9000);
  }
  delay(100);
  enterSleep(CLK_4_S);
}

void wakeUp(){
  // Bring XBee out of sleep mode
  digitalWrite(sleepPin, LOW);
  delay(WAKE_UP_TIME);
}

void sleep(){
  // Put Xbee into low energy state
  digitalWrite(sleepPin, HIGH);
}

void enterSleep(int time)
{
  if (time > 9){
    return;
  }else{
    setClock(time);
  }
  
  /* Don't forget to clear the flag. */
  f_wdt = 0;
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  
  /* Now enter sleep mode. */
  sleep_mode();
  
  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
  
  /* Re-enable the peripherals. */
  power_all_enable();
  
  delay(100);
}

void setClock(int time){
  byte timeVal = 0x00;
  
  if (time == 0){
      timeVal = 0; /* 0.016 seconds */
  }else if (time == 1){
      timeVal = 1<<WDP0; /* 0.032 seconds */
  }else if (time == 2){
      timeVal = 1<<WDP1; /* 0.064 seconds */
  }else if (time == 3){
      timeVal = 1<<WDP0 | 1<<WDP1; /* 0.125 seconds */
  }else if (time == 4){
      timeVal = 1<<WDP2; /* 0.25 seconds */
  }else if (time == 5){
      timeVal = 1<<WDP0 | 1<<WDP2; /* 0.5 seconds */
  }else if (time == 6){
      timeVal = 1<<WDP1 | 1<<WDP2; /* 1.0 second */
  }else if (time == 7){
      timeVal = 1<<WDP0 | 1<<WDP1 | 1>>WDP2; /* 2.0 seconds */
  }else if (time == 8){
      timeVal = 1<<WDP3; /* 4.0 seconds */
  }else{
      timeVal = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
  }
  
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  
  WDTCSR = timeVal;
  
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);
}

ISR(WDT_vect)
{
  if(f_wdt == 0)
  {
    f_wdt=1;
  }
}

void pinInterrupt(){
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
  const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}
