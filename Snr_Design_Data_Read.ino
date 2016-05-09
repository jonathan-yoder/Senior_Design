#include <Wire.h>
#include "HX711.h"
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <SD.h>

#define DOUT A3
#define CLK  A2

HX711 scale(DOUT, CLK);
Adafruit_MMA8451 mma = Adafruit_MMA8451();
const int chipSelect = 10;
int accPresent;
File dataFile;
float calibration_factor = 50000;
unsigned long tPrev;

void setup() {
  Serial.begin(9600);
  Serial.println("Started");

  SD.begin(chipSelect);
  dataFile = SD.open("datafile.txt");
  dataFile.close();
  
  accPresent = mma.begin();
  if (! accPresent) {
    Serial.println("Accelerometer not present");
  }else{
    mma.setRange(MMA8451_RANGE_4_G);
    Serial.println("Accelerometer found");
  }

  tPrev = millis();
}

void loop() {
  String dataString = "";
  dataString += String(millis());
  dataString += ',';
  dataString += String(scale.get_units(), 5);
  if (accPresent){
    dataString += ',';
    dataString += String(mma.x);
    dataString += ',';
    dataString += String(mma.y);
    dataString += ',';
    dataString += String(mma.z);
  }

  dataFile = SD.open("datafile.txt", FILE_WRITE);
  dataFile.println(dataString);
  dataFile.close();
  Serial.println(dataString);

  while(millis() < tPrev + (unsigned long)100);
  tPrev = millis();
}
