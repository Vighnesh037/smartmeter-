#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "ACS712.h"
#include <ZMPT101B.h>
#include "time.h"

/******** WIFI ********/
#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASSWORD "YOUR_PASSWORD"

/******** FIREBASE ********/
#define API_KEY "YOUR_API_KEY"
#define DATABASE_URL "YOUR_DATABASE_URL"
#define USER_EMAIL "test@test.com"
#define USER_PASSWORD "123456"

/******** LCD ********/
LiquidCrystal_I2C lcd(0x27, 16, 2);

/******** SENSORS ********/
ACS712 ACS(35, 3.3, 4095, 66);
ZMPT101B voltageSensor(34, 50.0);

/******** FIREBASE OBJECTS ********/
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/******** TIME CONFIG ********/
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

/******** VARIABLES ********/
float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;
float powerFactor = 0.9;

float hourlyEnergy[24] = {0};
float monthlyUnits = 0;
float monthlyBill = 0;
float tariff = 6.0;

int peak1 = -1;
int peak2 = -1;
int suggestedHour = -1;

unsigned long lastTime = 0;
unsigned long firebaseTimer = 0;
unsigned long peakTimer = 0;

/******** GET CURRENT HOUR ********/
int getCurrentHour() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return 0;
  return timeinfo.tm_hour;
}

/******** TARIFF FUNCTION ********/
float getTariff(int hour) {
  if(hour >= 18 && hour < 22) return 7;
  else if(hour >= 10 && hour < 18) return 5;
  else return 3;
}

/******** PEAK DETECTION ********/
void detectPeaks() {

  peak1 = 0;
  peak2 = 1;

  for(int i=0;i<24;i++) {

    if(hourlyEnergy[i] > hourlyEnergy[peak1]) {
      peak2 = peak1;
      peak1 = i;
    }
    else if(hourlyEnergy[i] > hourlyEnergy[peak2] && i != peak1) {
      peak2 = i;
    }
  }
}

/******** SCHEDULING ********/
void calculateScheduling() {

  float estimatedUnit = power / 1000.0;
  float minCost = 9999;

  for(int i=0;i<24;i++) {

    if(i == peak1 || i == peak2)
      continue;

    float cost = estimatedUnit * getTariff(i);

    if(cost < minCost) {
      minCost = cost;
      suggestedHour = i;
    }
  }
}

void setup() {

  Serial.begin(115200);
  Wire.begin(21,22);

  lcd.init();
  lcd.backlight();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  ACS.autoMidPoint();
  voltageSensor.setSensitivity(500.0f);

  lastTime = millis();
}

void loop() {

  unsigned long currentTime = millis();
  float timeDiff = (currentTime - lastTime) / 1000.0;

  /******** READ VOLTAGE ********/
  float v = voltageSensor.getRmsVoltage();
  voltage = (v > 80) ? v : 0;

  /******** READ CURRENT ********/
  float sum = 0;
  for(int i=0;i<300;i++){
    sum += ACS.mA_AC();
    delayMicroseconds(200);
  }
  float mA = sum / 300.0;
  current = (mA < 30) ? 0 : mA / 1000.0;

  /******** POWER ********/
  power = voltage * current * powerFactor;

  /******** ENERGY ********/
  float incrementalEnergy = (power * timeDiff) / 3600000.0;
  energy += incrementalEnergy;
  monthlyUnits += incrementalEnergy;
  monthlyBill = monthlyUnits * tariff;

  int hourNow = getCurrentHour();
  hourlyEnergy[hourNow] += incrementalEnergy;

  lastTime = currentTime;

  /******** RUN PEAK DETECTION EVERY HOUR ********/
  if(millis() - peakTimer > 3600000) {
    detectPeaks();
    calculateScheduling();
    peakTimer = millis();
  }

  /******** FIREBASE UPDATE EVERY 5 SEC ********/
  if(Firebase.ready() && millis() - firebaseTimer > 5000) {

    firebaseTimer = millis();

    Firebase.RTDB.setFloat(&fbdo, "/smart_socket/live/voltage", voltage);
    Firebase.RTDB.setFloat(&fbdo, "/smart_socket/live/current", current);
    Firebase.RTDB.setFloat(&fbdo, "/smart_socket/live/power", power);
    Firebase.RTDB.setFloat(&fbdo, "/smart_socket/live/energy", energy);

    Firebase.RTDB.setFloat(&fbdo, "/smart_socket/monthly/units", monthlyUnits);
    Firebase.RTDB.setFloat(&fbdo, "/smart_socket/monthly/bill", monthlyBill);

    for(int i=0;i<24;i++){
      String path = "/smart_socket/hourly/" + String(i);
      Firebase.RTDB.setFloat(&fbdo, path, hourlyEnergy[i]);
    }

    Firebase.RTDB.setInt(&fbdo, "/smart_socket/analytics/peak1", peak1);
    Firebase.RTDB.setInt(&fbdo, "/smart_socket/analytics/peak2", peak2);
    Firebase.RTDB.setInt(&fbdo, "/smart_socket/analytics/suggested_hour", suggestedHour);
  }

  delay(500);
}