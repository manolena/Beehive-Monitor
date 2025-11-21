#include "calibration.h"
#include "config.h"
#include <Preferences.h>
#include <Wire.h>

// HX711 library (bogde style)
#include "HX711.h"

// BME280 (use Adafruit BME280 for T/H/P)
#include <Adafruit_BME280.h>

// MPU6050 (Adafruit)
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// NOTE: config.h declares "extern Preferences prefs;" so we use that global instance.
// Do NOT declare another Preferences prefs here (that caused the prior compile error).

// HX711 instance using pins from config.h
static HX711 hx;
static bool hxReady = false;

// BME280 instance (I2C)
static Adafruit_BME280 bme;
static bool bmeReady = false;

// MPU6050 instance
static Adafruit_MPU6050 mpu;
static bool mpuReady = false;

// Preference keys
static const char* NS = "calib";
static const char* K_ZERO = "scale_zero";
static const char* K_SCALE = "scale_fac";
static const char* K_BATT_FACTOR = "batt_fac";
static const char* K_ACCEL_BX = "acc_bx";
static const char* K_ACCEL_BY = "acc_by";
static const char* K_ACCEL_BZ = "acc_bz";
static const char* K_TEMP_OFF = "temp_off";
static const char* K_HUM_OFF = "hum_off";

void calibration_init() {
  // Use the global prefs instance; ensure someone defines it once in the project.
  prefs.begin(NS, false);

  // HX711 begin
  hx.begin(DOUT, SCK);
  // Allow HX711 to stabilize briefly
  delay(100);
  hxReady = true;

  // BME begin
  if (bme.begin(0x76)) {
    bmeReady = true;
  } else if (bme.begin(0x77)) {
    bmeReady = true;
  } else {
    bmeReady = false;
  }

  // MPU begin
  if (mpu.begin()) {
    mpuReady = true;
  } else {
    mpuReady = false;
  }
}

// Helper: average N HX711 raw readings
static long hx_readAverage(uint8_t samples, uint16_t delayMs) {
  long long sum = 0;
  uint8_t cnt = 0;
  for (uint8_t i = 0; i < samples; ++i) {
    long r = hx.read();
    sum += (long long)r;
    cnt++;
    delay(delayMs);
  }
  if (cnt == 0) return 0;
  return (long)(sum / cnt);
}

bool calibration_tareScale(uint8_t samples, uint16_t delayMs) {
  if (!hxReady) return false;
  // Read baseline average
  long zero = hx_readAverage(samples, delayMs);
  prefs.putLong(K_ZERO, zero);
  // If no scale factor exists, set to a default (avoid divide-by-zero)
  float existing = prefs.getFloat(K_SCALE, NAN);
  if (isnan(existing)) {
    prefs.putFloat(K_SCALE, 1.0f);
  }
  return true;
}

long calibration_readScaleRaw(uint8_t samples, uint16_t delayMs) {
  if (!hxReady) return 0;
  return hx_readAverage(samples, delayMs);
}

bool calibration_calibrateOnePoint(float knownWeightKg, uint8_t samples, uint16_t delayMs) {
  if (!hxReady) return false;
  long zero = prefs.getLong(K_ZERO, 0);
  long rawKnown = hx_readAverage(samples, delayMs);

  if (rawKnown == zero) return false; // invalid
  float scale = knownWeightKg / float(rawKnown - zero);
  prefs.putFloat(K_SCALE, scale);
  // Good to store zero too (already stored by tare)
  prefs.putLong(K_ZERO, zero);
  return true;
}

float calibration_computeWeightFromRaw(long raw) {
  long zero = prefs.getLong(K_ZERO, 0);
  float scale = prefs.getFloat(K_SCALE, 0.0f);
  if (scale == 0.0f) return 0.0f;
  return float(raw - zero) * scale;
}

bool calibration_getScaleParams(long &zeroRaw, float &scaleFactor) {
  zeroRaw = prefs.getLong(K_ZERO, 0);
  scaleFactor = prefs.getFloat(K_SCALE, 0.0f);
  return (scaleFactor != 0.0f);
}

// -----------------------
// Battery calibration
// -----------------------
bool calibration_calibrateBattery(float knownVoltage, uint8_t samples, uint16_t delayMs) {
  // read ADC samples on BATTERY_PIN
  long sum = 0;
  for (uint8_t i = 0; i < samples; ++i) {
    int raw = analogRead(BATTERY_PIN);
    sum += raw;
    delay(delayMs);
  }
  float avgRaw = float(sum) / float(samples);
  // ADC range 0..4095 for ESP32; vRef assumed 3.3V
  const float ADC_MAX = 4095.0f;
  const float VREF = 3.3f;
  float measuredV = (avgRaw / ADC_MAX) * VREF;
  // Voltage divider factor from config R1,R2 (Vbat = Vmeas * (R1+R2)/R2)
  float divider = (R1 + R2) / R2;
  float vbatMeasured = measuredV * divider;
  // Correction factor = true / measured
  float factor = 1.0f;
  if (vbatMeasured > 0.0001f) factor = knownVoltage / vbatMeasured;
  prefs.putFloat(K_BATT_FACTOR, factor);
  return true;
}

float calibration_readBatteryVoltage(uint8_t samples, uint16_t delayMs) {
  long sum = 0;
  for (uint8_t i = 0; i < samples; ++i) {
    sum += analogRead(BATTERY_PIN);
    delay(delayMs);
  }
  float avgRaw = float(sum) / float(samples);
  const float ADC_MAX = 4095.0f;
  const float VREF = 3.3f;
  float measuredV = (avgRaw / ADC_MAX) * VREF;
  float divider = (R1 + R2) / R2;
  float vbatMeasured = measuredV * divider;
  float factor = prefs.getFloat(K_BATT_FACTOR, 1.0f);
  return vbatMeasured * factor;
}

// -----------------------
// ACCEL calibration
// -----------------------
bool calibration_calibrateAccelZero(uint8_t samples, uint16_t delayMs) {
  if (!mpuReady) return false;
  float bx = 0, by = 0, bz = 0;
  sensors_event_t a, g, temp;
  for (uint8_t i = 0; i < samples; ++i) {
    mpu.getEvent(&a, &g, &temp);
    bx += a.acceleration.x;
    by += a.acceleration.y;
    bz += a.acceleration.z;
    delay(delayMs);
  }
  bx /= float(samples);
  by /= float(samples);
  bz /= float(samples);
  prefs.putFloat(K_ACCEL_BX, bx);
  prefs.putFloat(K_ACCEL_BY, by);
  prefs.putFloat(K_ACCEL_BZ, bz);
  return true;
}

void calibration_getAccelBias(float &bx, float &by, float &bz) {
  bx = prefs.getFloat(K_ACCEL_BX, 0.0f);
  by = prefs.getFloat(K_ACCEL_BY, 0.0f);
  bz = prefs.getFloat(K_ACCEL_BZ, 0.0f);
}

// -----------------------
// BME temp/hum offsets
// -----------------------
void calibration_setTempOffset(float deltaC) {
  prefs.putFloat(K_TEMP_OFF, deltaC);
}

void calibration_setHumOffset(float deltaPct) {
  prefs.putFloat(K_HUM_OFF, deltaPct);
}

float calibration_getTempOffset() {
  return prefs.getFloat(K_TEMP_OFF, 0.0f);
}

float calibration_getHumOffset() {
  return prefs.getFloat(K_HUM_OFF, 0.0f);
}

// Summary for UI
String calibration_getSummary() {
  long zero = prefs.getLong(K_ZERO, 0);
  float scale = prefs.getFloat(K_SCALE, 0.0f);
  float battFac = prefs.getFloat(K_BATT_FACTOR, 1.0f);
  float tb = prefs.getFloat(K_TEMP_OFF, 0.0f);
  float hb = prefs.getFloat(K_HUM_OFF, 0.0f);
  char buf[256];
  snprintf(buf, sizeof(buf),
           "Zero:%ld Scale:%.6f Bfac:%.4f Toff:%.2f Hoff:%.2f",
           zero, scale, battFac, tb, hb);
  return String(buf);
}