#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>               // provide WiFi symbols (WiFi, WL_CONNECTED) to modules
#include <LiquidCrystal_I2C.h>  // provide LCD type and allow extern declaration

// Timing
#define MEASUREMENT_INTERVAL  (3600ULL * 1000000ULL)

// =============================
// Fixed hardware pinout
// =============================

// I2C for LCD and Sensors
#define SDA_PIN        21
#define SCL_PIN        22

// HX711 Load Cell
#define DOUT           19
#define SCK            18

// SD Card
#define SD_MISO        2
#define SD_MOSI        15
#define SD_SCLK        14
#define SD_CS          13

// Battery
#define BATTERY_PIN    35
#define R1             10000.0
#define R2             10000.0

// LTE Modem
#define MODEM_RX       27
#define MODEM_TX       26
#define MODEM_PWR      4

// Buttons
#define BTN_UP         23
#define BTN_DOWN       12
#define BTN_SELECT     33
#define BTN_BACK       32

// Connectivity modes
#define CONNECTIVITY_LTE     0
#define CONNECTIVITY_WIFI    1
#define CONNECTIVITY_OFFLINE 2

// LTE Configuration
#define MODEM_APN        "internet"
#define MODEM_GPRS_USER  ""
#define MODEM_GPRS_PASS  ""

// -----------------------------------------------------------------------------
// Use Open-Meteo as the default weather provider (no API key required).
// Set to 0 to keep previous OpenWeather code paths.
#define USE_OPENMETEO 1

// ---------------------------
// Dual WiFi compile-time defaults
// Primary network (SSID1) and Secondary network (SSID2)
// These were provided by the user and are now hardcoded here.
#ifndef WIFI_SSID1
#define WIFI_SSID1 "Redmi Note 13"
#define WIFI_PASS1 "nen57asz5g44sh2"
#endif

#ifndef WIFI_SSID2
#define WIFI_SSID2 "COSMOTE-32bssa"
#define WIFI_PASS2 "vudvvc5x97s4afpk"
#endif

// Single global Preferences instance is defined in one .cpp (weather_manager.cpp).
extern Preferences prefs;
extern int connectivityMode;

// Single global LCD instance is defined in ui.cpp; make it visible to all units.
extern LiquidCrystal_I2C lcd;

#define INIT_SD_CARD() do { \
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS); \
    SD.begin(SD_CS); \
} while(0)

// ============================================================
// TEMPORARY PLACEHOLDER SENSOR VALUES (TESTING ONLY)
// ============================================================

// Weight (kg)
extern float test_weight;

// Internal Temperature & Humidity (SI7021)
extern float test_temp_int;
extern float test_hum_int;

// External Temperature, Humidity, Pressure (BME280/BME680)
extern float test_temp_ext;
extern float test_hum_ext;
extern float test_pressure;

// Accelerometer (X,Y,Z)
extern float test_acc_x;
extern float test_acc_y;
extern float test_acc_z;

// Battery voltage & percentage
extern float test_batt_voltage;
extern int   test_batt_percent;

// Modem signal
extern int   test_rssi;

// ------------------------
// Default location (compile-time fallback only)
// The preferred method is to store coords at runtime in Preferences.
// These macros act as compile-time fallbacks.
#define DEFAULT_LAT       37.983810       // fallback: Athens latitude
#define DEFAULT_LON       23.727539       // fallback: Athens longitude

// -----------------------------------------------------------------------------
// Optional behavior toggles (local to project)
// If you want the HTTP key-server to auto-start when WiFi connects, leave AUTOSTART_KEYSERVER=1.
// Set to 0 to require manual server start (menu action) instead.
#ifndef AUTOSTART_KEYSERVER
#define AUTOSTART_KEYSERVER 1
#endif