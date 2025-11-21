#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>

// Initialize calibration subsystem (starts sensors used for calibration)
void calibration_init();

// SCALE (HX711) APIs
// Tare: measure zero baseline and store it in Preferences
bool calibration_tareScale(uint8_t samples = 32, uint16_t delayMs = 20);
// Read average raw HX711 value (not adjusted)
long calibration_readScaleRaw(uint8_t samples = 32, uint16_t delayMs = 10);
// Compute 1-point calibration factor and store it: knownWeightKg / (rawKnown - rawZero)
bool calibration_calibrateOnePoint(float knownWeightKg, uint8_t samples = 32, uint16_t delayMs = 10);

// Retrieve computed weight (kg) from a raw reading using saved factor
float calibration_computeWeightFromRaw(long raw);

// Return stored scale parameters
bool calibration_getScaleParams(long &zeroRaw, float &scaleFactor);

// BATTERY calibration: provide known voltage (measured with meter) to calculate correction factor
bool calibration_calibrateBattery(float knownVoltage, uint8_t samples = 8, uint16_t delayMs = 20);
float calibration_readBatteryVoltage(uint8_t samples = 8, uint16_t delayMs = 10);

// ACCEL (MPU6050) calibration (zero/bias capture)
bool calibration_calibrateAccelZero(uint8_t samples = 128, uint16_t delayMs = 5);
void calibration_getAccelBias(float &bx, float &by, float &bz);

// BME/BMP offsets
void calibration_setTempOffset(float deltaC);
void calibration_setHumOffset(float deltaPct);
float calibration_getTempOffset();
float calibration_getHumOffset();

// Persisted state introspection (for UI)
String calibration_getSummary();

#endif // CALIBRATION_H