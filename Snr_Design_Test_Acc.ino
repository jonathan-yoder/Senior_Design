/* The code below is designed to test that the MMA8451
 * accelerometer is capable of waking the Arduino Pro Mini
 * from a low-power state when a certain acceleration is
 * experienced.
 * When movement is not detected, the system will sleep
 * until movement causes an interrupt to wake it up.
 * When the system wakes, it will send a message and then
 * sleep for 8 seconds on a watch-dog timer. When the timer
 * wakes it, it will check again whether or not it has moved
 * recently.
 */ 

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

#define SLEEP_PIN_TRANS -1

#define interruptPinTrans 2

#define CLK_16_MS 0
#define CLK_32_MS 1
#define CLK_64_MS 2
#define CLK_125_MS 3
#define CLK_250_MS 4
#define CLK_500_MS 5
#define CLK_1_S 6
#define CLK_2_S 7
#define CLK_4_S 8
#define CLK_8_S 9

const double odrRateHz[] = {800, 400, 200, 100, 50, 12.5, 6.25, 1.56};

mma8451_range_t mmaScale = MMA8451_RANGE_2_G;
mma8451_dataRate_t mmaODR = MMA8451_DATARATE_12_5_HZ;
/* The axes on which movement should be detected */
byte axesToDetect = 0b111;
/* The minimum acceleration threshold at which to send an interrupt */
byte mmaSensitivity = 0x04;
/* The amount of time the threshold must be met before an interrupt is sent */
byte mmaDebounce = 0x02;
volatile int f_wdt=1;

const byte address = 0x1D;
Adafruit_MMA8451 accel;

void setup() {
  Serial.begin(9600);
  pinMode(interruptPinTrans, INPUT);
  accel.begin(address);
  
  setUpMMA();

}

void loop() {
  if (!digitalRead(interruptPinTrans) && didMove()){
    Serial.println("Moving");
    delay(100);
    Serial.println("Sleeping on WDT");
    delay(100);
    enterSleep(CLK_8_S);
  }else{
    Serial.println("Not moving, sleep on acc");
    delay(100);
    enterSleep(SLEEP_PIN_TRANS);
  }
}

void enterSleep(int time)
{
  if (time < 0){    
    attachInterrupt(0, pinInterrupt, FALLING);
  }else if (time > 9){
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
  
  if (time < 0){
    detachInterrupt(0);
  }
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

boolean didMove(){
  boolean moved = ((IIC_RegRead(0x0C) & 0x20) == 0x20);
  if (moved){
    IIC_RegRead(0x1E);
  }
  return moved;
}

void setUpMMA(){
  IIC_RegWrite(0x2A, (mmaODR << 3));
  IIC_RegWrite(0x2A, (mmaODR << 3));
  IIC_RegWrite(0x1D, (1<<4 | axesToDetect<<1));
  IIC_RegWrite(0x1F, mmaSensitivity);
  IIC_RegWrite(0x20, mmaDebounce);
  IIC_RegWrite(0x0E, mmaScale);
  IIC_RegWrite(0x09, 0x00);
  IIC_RegWrite(0x2D, 0x20);  
  IIC_RegWrite(0x2E, 0x20);
  
  IIC_RegWrite(0x2A, (IIC_RegRead(0x2A) | 0x01));
}

void IIC_RegWrite(byte reg, byte data){
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission(); //Stop transmitting
}

byte IIC_RegRead(byte reg){
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false); //endTransmission but keep the connection active

  Wire.requestFrom(address, (byte) 1); //Ask for 1 byte, once done, bus is released by default

  while(!Wire.available()) ; //Wait for the data to come back

  return Wire.read(); //Return this one byte
}
