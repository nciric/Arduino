//#include "air_quality.h"
#include "PMS.h"

// Seven segment display digits.
// These are for common anode (low). If you need common
// cathode, just invert value in print_digit method.
int digit_array[10][7] = {
  { 0, 0, 0, 0, 0, 0, 1 }, // 0
  { 1, 0, 0, 1, 1, 1, 1 }, // 1
  { 0, 0, 1, 0, 0, 1, 0 }, // 2
  { 0, 0, 0, 0, 1, 1, 0 }, // 3
  { 1, 0, 0, 1, 1, 0, 0 }, // 4
  { 0, 1, 0, 0, 1, 0, 0 }, // 5
  { 0, 1, 0, 0, 0, 0, 0 }, // 6
  { 0, 0, 0, 1, 1, 1, 1 }, // 7
  { 0, 0, 0, 0, 0, 0, 0 }, // 8
  { 0, 0, 0, 1, 1, 0, 0 }  // 9
};

// Digit selector pins.
int DS1 = 9;
int DS2 = 10;
int DS3 = 11;

// Holds AQI range values from
// https://en.wikipedia.org/wiki/Air_quality_index#Indices_by_location (US)
typedef struct {
  float c_low;
  float c_high;
  int i_low;
  int i_high;
} Aqi;

// Keep a sliding window of values to stabilize the reading.
// First PM_WINDOW_LEN cycles will give bogus result, since average is skewed by 0.0 values.
const int PM_WINDOW_LEN = 20;
float pm_2_5_window[PM_WINDOW_LEN] = {0.0};
float pm_10_0_window[PM_WINDOW_LEN] = {0.0};

PMS pms(Serial1);
PMS::DATA data;

void setup() {
  // Setup 3x7 segment display.
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(DS1, OUTPUT);
  pinMode(DS2, OUTPUT);
  pinMode(DS3, OUTPUT);

  // Wait for serial monitor to open
  Serial.begin(9600);
  Serial1.begin(9600);
  // This one is only for debugging on PC. We'll ignore it
  // if it fails to boot within 4s.
  while (!Serial && millis() < 4000) {};
  // This one talks to the sensor, so we have to wait for it.
  while (!Serial1) delay(10);
  Serial.println("PMSX003 Air Quality Sensor");

  // Wait one second for sensor to boot up!
  delay(1000);
}

void loop() {
  if (pms.read(data)) {
    Serial.println("Dust Concentration");
    Serial.println("PM1.0 :" + String(data.PM_AE_UG_1_0) + "(ug/m3)");
    Serial.println("PM2.5 :" + String(data.PM_AE_UG_2_5) + "(ug/m3)");
    Serial.println("PM10  :" + String(data.PM_AE_UG_10_0) + "(ug/m3)");
    update_2_5_window(data.PM_AE_UG_2_5);
    update_10_0_window(data.PM_AE_UG_10_0);
  }

  // Make all measurements, then take max AQI value.
  int aqi_2_5_value = aqi_2_5(average_2_5());
  int aqi_10_0_value = aqi_10(average_10_0());
  print_number(max(aqi_2_5_value, aqi_10_0_value));

  // We have to be fast here, to avoid display issues.
  delay(1);
}

// AQI value to be shown on display.
int get_aqi(Aqi, float);

float aqi_2_5(int pm_ug_2_5) {
  Aqi table[] = {
    {0.0, 12.0, 0, 50},
    {12.1, 35.4, 51, 100},
    {35.5, 55.4, 101, 150},
    {55.5, 150.4, 151, 200},
    {150.5, 250.4, 201, 300},
    {250.5, 350.4, 301, 400},
    {350.5, 500.4, 401, 500},
  };
  int len = sizeof(table)/sizeof(table[0]);
  for (int i = 0; i < len; ++i) {
    if (table[i].c_low <= pm_ug_2_5 && pm_ug_2_5 <= table[i].c_high) {
      return get_aqi(table[i], pm_ug_2_5);
    }
  }
  // It's not air we breath anymore...
  return 666;
}

float aqi_10(int pm_ug_10_0) {
  Aqi table[] = {
    {0.0, 54.0, 0, 50},
    {54.1, 154.0, 51, 100},
    {154.1, 254.0, 101, 150},
    {254.1, 354.0, 151, 200},
    {354.1, 424.0, 201, 300},
    {424.1, 504.0, 301, 400},
    {504.1, 604.0, 401, 500},
  };
  int len = sizeof(table)/sizeof(table[0]);
  for (int i = 0; i < len; ++i) {
    if (table[i].c_low <= pm_ug_10_0 && pm_ug_10_0 <= table[i].c_high) {
      return get_aqi(table[i], pm_ug_10_0);
    }
  }
  // It's not air we breath anymore...
  return 666;
}

int get_aqi(Aqi aqi, float reading) {
  // Formula from https://en.wikipedia.org/wiki/Air_quality_index#Indices_by_location
  // US region.
  float result = (aqi.i_high - aqi.i_low) * (reading - aqi.c_low) / (aqi.c_high - aqi.c_low) + aqi.i_low;
  if (result < 0.0) {
    return 0;
  } else {
    return static_cast<int>(result);
  }
}

// Three digit number.
void print_number(int number) {
  print_digit(number % 10, DS3 - DS1);
  number /= 10;
  delay(1);
  print_digit(number % 10, DS2 - DS1);
  number /= 10;
  delay(1);
  print_digit(number % 10, DS1 - DS1);
}

// Position = 0 for first digit, etc.
void print_digit(int digit, int position) {
  int pin = 2;  // we start with pin 2 for digits.

  select_position(position);
  for (int i = 0; i < 7; ++i, ++pin) {
    digitalWrite(pin, digit_array[digit][i]);
  }
}

void select_position(int position) {
  digitalWrite(DS1, LOW);
  digitalWrite(DS2, LOW);
  digitalWrite(DS3, LOW);
  digitalWrite(position + DS1, HIGH);
}

void update_2_5_window(float value) {
  static int cursor = 0;
  pm_2_5_window[cursor] = value;
  if (cursor >= PM_WINDOW_LEN) {
    cursor = 0;
  } else {
    cursor++;
  }
}

void update_10_0_window(float value) {
  static int cursor = 0;
  pm_10_0_window[cursor] = value;
  if (cursor >= PM_WINDOW_LEN) {
    cursor = 0;
  } else {
    cursor++;
  }
}

float average_2_5() {
  float sum = 0.0;
  for (int i = 0; i < PM_WINDOW_LEN; ++i) {
    sum += pm_2_5_window[i];
  }
  return sum / PM_WINDOW_LEN;
}

float average_10_0() {
  float sum = 0.0;
  for (int i = 0; i < PM_WINDOW_LEN; ++i) {
    sum += pm_10_0_window[i];
  }
  return sum / PM_WINDOW_LEN;
}
