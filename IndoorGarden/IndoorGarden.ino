// Includes
#include <EEPROM.h>
#include <Arduino.h>
#include <OneWireNg.h>
#include <DallasTemperature.h>
#include "RunningAverage.h"
#include "Ticker.h"

// Pin assignments
#define ONE_WIRE_BUS 7
#define TDS_SENSOR_PIN 32
#define TDS_SENSOR_POWER_PIN 6
#define WATER_LEVEL_POWER_PIN 4
#define WATER_LEVEL_SIGNAL_PIN 35
#define PH_SENSOR_PIN 34
#define PH_UP_RELAY_PIN 5
#define PH_DOWN_RELAY_PIN 8

// Constants
int ph_samples = 10;

//Fudge Factors
//PH Calibration value
float ph_calibration_value = 19.1;

// Sensor objects
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Current sensor values
float tdsValue = 0;
float ecValue = 0;
float waterLevel = 0;
RunningAverage phValues(10);
float phValue = 0;

// Default target values
int waterLevelMin = 300;
float targetpH = 6.0;
float phMargin = 0.2;


float tdsMin = 300;
float ecMin = 0.3;

float tdsMax = 1500;
float ecMax = 1.4;

float getWaterLevel();
float getpH();
float getTDS();
void Alert(String message);

void phUp();
void phDown();

int phWait = 0;
int phWaitCycles = 40;

void setup() {
  Serial.begin(115200);
  sensors.begin();

  pinMode(WATER_LEVEL_POWER_PIN, OUTPUT);    // configure D7 pin as an OUTPUT
  pinMode(TDS_SENSOR_POWER_PIN, OUTPUT);     // configure D6 pin as an OUTPUT
  digitalWrite(WATER_LEVEL_POWER_PIN, LOW);  // turn the sensor OFF
  digitalWrite(TDS_SENSOR_POWER_PIN, LOW);   // turn the sensor OFF

  phValues.clear();
}

void loop() {

  // Update all the sensors with fresh, tasty values
  waterLevel = getWaterLevel();
  phValue = getpH();
  tdsValue = getTDS();
  ecValue = (tdsValue * 2.0) / 1000.0;

  // Control logic
  if (waterLevel < waterLevelMin) {  // If we are low on water, don't adjust anything. We will be adding or changing water soon (hopefully)
    Alert("Water Level OOR");
  } else {
    if (phWait > 0) {
      phWait--;
    } else {
      if ((targetpH - phMargin) > phValue) {  // pH out of range (too low)
        phUp();
        phWait = phWaitCycles;
      } else if ((targetpH + phMargin) < phValue) {  // pH out of range (too high)
        phDown();
        phWait = phWaitCycles;
      } else {  // pH is good. Check TDS
        if (ecValue < ecMin) {
          Alert("TDS OOR");
        }
      }
    }
  }

  Serial.print(tdsValue, 0);
  Serial.println("ppm");
  Serial.print("EC: ");
  Serial.println(ecValue, 1);
  //  Serial.print("Temperature is: ");
  //  Serial.print(tempC);
  //  Serial.print(" (");
  //  Serial.print(tempF);
  //  Serial.println(" F)");
  Serial.print("pH: ");
  Serial.println(phValue);
  Serial.println("");

  delay(1000);
}

float getWaterLevel() {
  digitalWrite(WATER_LEVEL_POWER_PIN, HIGH);    // turn the sensor ON
  delay(10);                                    // wait 10 milliseconds
  int wl = analogRead(WATER_LEVEL_SIGNAL_PIN);  // read the analog value from sensor
  digitalWrite(WATER_LEVEL_POWER_PIN, LOW);     // turn the sensor OFF
  return wl;
}

float ph(float voltage) {
  return 7 + ((2.5 - voltage) / 0.18);
}

float getpH() {
  int measurings = 0;
  for (int i = 0; i < ph_samples; i++) {
    measurings += analogRead(PH_SENSOR_PIN);
    delay(10);
  }
  float voltage = 5 / 1024.0 * measurings / ph_samples;
  Serial.println(voltage);
  float ph_act = ph(voltage);
  phValues.addValue(ph_act);
  return phValues.getAverage();
}

void Alert(String message = "") {
  Serial.println("----ALERT----");
  Serial.println(message);
}

float getWaterTemp(bool f = false) {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  float tempF = sensors.getTempFByIndex(0);
  if (f) return tempF;
  else
    return tempC;
}

float getTDS() {
  digitalWrite(TDS_SENSOR_POWER_PIN, HIGH);
  delay(100);
  float tempC = getWaterTemp();      // For DEBUG!
  digitalWrite(TDS_SENSOR_POWER_PIN, LOW);
  return 0;  // then get the value
}

void phUp() {
  Serial.println("PH UP!");
  digitalWrite(PH_UP_RELAY_PIN, HIGH);
  delay(500);
  digitalWrite(PH_UP_RELAY_PIN, LOW);
  return;
}

void phDown() {
  Serial.println("PH DOWN!");
  digitalWrite(PH_DOWN_RELAY_PIN, HIGH);
  delay(500);
  digitalWrite(PH_DOWN_RELAY_PIN, LOW);
  return;
}
