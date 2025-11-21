#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>

struct WeatherDay {
  String date;        // used as "DD-MM HH:MM" for 6-hour samples
  float temp_min;     // for 6h sample: temperature at sample time
  float temp_max;
  float humidity;     // relative humidity (%) for the sample
  float pressure;     // surface pressure in hPa for the sample
  String desc;        // short description (e.g. "Light rain" or "Clear")
};

// Init the module (call from setup)
void weather_init();

// Fetch weather now (blocking). Returns true on success.
// When USE_OPENMETEO is enabled, this fetches 6-hourly samples for the next 3 days.
bool weather_fetch();

// Accessors
bool weather_hasData();
int  weather_daysCount();               // number of samples currently cached (e.g. 12)
void weather_getDay(int idx, WeatherDay &out); // idx: 0..weather_daysCount()-1

// Runtime helpers (no API key functions anymore)
void weather_setCoords(double lat, double lon);          // store coordinates in Preferences
bool weather_geocodeLocation(const char* city, const char* countryCode); // fetch lat/lon by name and store

String weather_getLastError();           // last error string

// Temporary debug helper (prints coords/URL and triggers fetch)
bool weather_debug_dumpAndFetch();

#endif // WEATHER_MANAGER_H