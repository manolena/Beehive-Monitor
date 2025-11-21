/*
  weather_manager.cpp  (Open‑Meteo implementation - humidity & pressure)

  Self-contained implementation that:
  - Uses Preferences namespace "beehive" to store lat/lon and place name/country.
  - Fetches hourly arrays from Open‑Meteo and samples every 6 hours for 72 hours.
  - Parses JSON using ArduinoJson v7 API (DeserializationError / deserializeJson).
  - Exposes weather_init(), weather_fetch(), weather_hasData(), weather_daysCount(),
    weather_getDay(), weather_setCoords(), weather_geocodeLocation(), weather_getLastError(),
    and weather_debug_dumpAndFetch() as in weather_manager.h.

  Make sure ArduinoJson 7.x is installed in the IDE.
*/

#include "config.h"
#include "weather_manager.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

Preferences prefs;

static const char *PREF_NS = "beehive";
static const char *PREF_KEY_LAT = "owm_lat";
static const char *PREF_KEY_LON = "owm_lon";
static const char *PREF_KEY_LOC_NAME = "loc_name";
static const char *PREF_KEY_LOC_COUNTRY = "loc_country";

static double s_lat = DEFAULT_LAT;
static double s_lon = DEFAULT_LON;
static bool   s_hasData = false;
static String s_lastError = "";

static WeatherDay *s_days = nullptr;
static int s_daysCount = 0;

// URL-encode helper (RFC3986-ish). Returns encoded string.
static String urlEncode(const String &str) {
  String encoded;
  encoded.reserve(str.length() * 3);
  const char *p = str.c_str();
  for (; *p; ++p) {
    unsigned char c = (unsigned char)*p;
    if ( (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') ||
         c == '-' || c == '_' || c == '.' || c == '~' ) {
      encoded += (char)c;
    } else if (c == ' ') {
      encoded += '+'; // form-style
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}

void weather_init() {
  prefs.begin(PREF_NS, false);
  String latS = prefs.getString(PREF_KEY_LAT, "");
  String lonS = prefs.getString(PREF_KEY_LON, "");
  if (latS.length() && lonS.length()) {
    s_lat = latS.toDouble();
    s_lon = lonS.toDouble();
  } else {
    s_lat = DEFAULT_LAT;
    s_lon = DEFAULT_LON;
  }
  s_hasData = false;
  s_lastError = "";
  if (s_days) { delete[] s_days; s_days = nullptr; s_daysCount = 0; }
}

// Store coords as strings (persist)
void weather_setCoords(double lat, double lon) {
  prefs.begin(PREF_NS, false);
  prefs.putString(PREF_KEY_LAT, String(lat, 6));
  prefs.putString(PREF_KEY_LON, String(lon, 6));
  prefs.end();
  s_lat = lat;
  s_lon = lon;
  Serial.print("[Weather] coords saved: lat=");
  Serial.print(lat, 6);
  Serial.print(" lon=");
  Serial.println(lon, 6);
}

String weather_getLastError() {
  return s_lastError;
}

// Map Open-Meteo weathercode to short description
static String mapWeatherCodeOpenMeteo(int code) {
  switch(code) {
    case 0: return "Clear";
    case 1: case 2: case 3: return "Partly cloudy";
    case 45: case 48: return "Fog";
    case 51: case 53: case 55: return "Drizzle";
    case 61: case 63: case 65: return "Rain";
    case 71: case 73: case 75: return "Snow";
    case 80: case 81: case 82: return "Showers";
    case 95: case 96: case 99: return "Thunder";
    default: return "N/A";
  }
}

// Geocode via Open-Meteo geocoding API, store coords and place name/country in prefs on success
bool weather_geocodeLocation(const char* city, const char* countryCode) {
  if (!city) return false;
  String q = String(city);
  if (countryCode && strlen(countryCode) > 0) {
    q += ",";
    q += String(countryCode);
  }
  String url = String("https://geocoding-api.open-meteo.com/v1/search?name=") + urlEncode(q) + "&count=1&language=en&format=json";
  Serial.print("[Weather] Geocode (OpenMeteo) -> ");
  Serial.println(url);

  if (WiFi.status() != WL_CONNECTED) {
    s_lastError = "WiFi not connected";
    Serial.println("[Weather] Geocode failed: WiFi not connected");
    return false;
  }

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  String body;
  if (code > 0) body = http.getString();
  http.end();

  if (code != 200) {
    s_lastError = String("HTTP_") + String(code) + ": " + body;
    Serial.print("[Weather] Geocode failed: ");
    Serial.println(s_lastError);
    return false;
  }

  // Parse JSON with ArduinoJson v7 API
  const size_t CAP = 10 * 1024;
  DynamicJsonDocument doc(CAP);
  DeserializationError derr = deserializeJson(doc, body);
  if (derr) {
    s_lastError = "JSON parse error";
    Serial.print("[Weather] Geocode JSON parse failed: ");
    Serial.println(derr.c_str());
    return false;
  }

  if (!doc.containsKey("results") || doc["results"].size() == 0) {
    s_lastError = "No geocode result";
    Serial.println("[Weather] Geocode no result");
    return false;
  }

  JsonObject r = doc["results"][0].as<JsonObject>();
  double lat = r["latitude"] | 0.0;
  double lon = r["longitude"] | 0.0;
  if (lat == 0.0 && lon == 0.0) {
    s_lastError = "Invalid coords";
    Serial.println("[Weather] Geocode invalid coords");
    return false;
  }

  // Persist coords
  weather_setCoords(lat, lon);

  // Persist place name & country for UI display
  String placeName = "";
  String country = "";
  if (r.containsKey("name"))    placeName = String((const char*)r["name"].as<const char*>());
  if (r.containsKey("country")) country   = String((const char*)r["country"].as<const char*>());

  prefs.begin(PREF_NS, false);
  if (placeName.length()) prefs.putString(PREF_KEY_LOC_NAME, placeName);
  if (country.length())   prefs.putString(PREF_KEY_LOC_COUNTRY, country);
  prefs.end();

  s_lastError = "";
  Serial.printf("[Weather] Geocode OK: lat=%.6f lon=%.6f", lat, lon);
  if (placeName.length()) {
    Serial.print(" place=");
    Serial.print(placeName);
    if (country.length()) { Serial.print(", country="); Serial.print(country); }
  }
  Serial.println();
  return true;
}

// Fetch Open-Meteo forecast: hourly arrays, sample every 6 hours for next 72h
static bool weather_fetch_open_meteo() {
  // include humidity and surface_pressure in hourly arrays
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=")
    + String(s_lat, 6) + "&longitude=" + String(s_lon, 6)
    + "&hourly=temperature_2m,weathercode,relativehumidity_2m,surface_pressure&forecast_days=3&timezone=auto";

  Serial.print("[Weather] OpenMeteo Request URL: ");
  Serial.println(url);

  if (WiFi.status() != WL_CONNECTED) {
    s_lastError = "WiFi not connected";
    Serial.println("[Weather] OpenMeteo fetch failed: WiFi not connected");
    return false;
  }

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  String body;
  if (code > 0) body = http.getString();
  http.end();

  if (code != 200) {
    s_lastError = String("HTTP_") + String(code) + ": " + body;
    Serial.print("[Weather] OpenMeteo HTTP fail: ");
    Serial.println(s_lastError);
    Serial.println(body.substring(0, min((int)body.length(), 512)));
    return false;
  }

  // Parse JSON
  const size_t CAP = 28 * 1024; // adjust if memory issues appear
  DynamicJsonDocument doc(CAP);
  DeserializationError derr = deserializeJson(doc, body);
  if (derr) {
    s_lastError = "JSON parse failed";
    Serial.print("[Weather] OpenMeteo JSON parse failed: ");
    Serial.println(derr.c_str());
    return false;
  }

  if (!doc.containsKey("hourly")) {
    s_lastError = "No hourly data";
    Serial.println("[Weather] No hourly data in OpenMeteo response");
    return false;
  }

  JsonObject hourly = doc["hourly"].as<JsonObject>();
  if (!hourly.containsKey("time") || !hourly.containsKey("temperature_2m")) {
    s_lastError = "Incomplete hourly data";
    Serial.println("[Weather] Hourly data incomplete");
    return false;
  }

  JsonArray times = hourly["time"].as<JsonArray>();
  JsonArray temps = hourly["temperature_2m"].as<JsonArray>();
  JsonArray codes = hourly["weathercode"].as<JsonArray>();
  JsonArray hums  = hourly.containsKey("relativehumidity_2m") ? hourly["relativehumidity_2m"].as<JsonArray>() : JsonArray();
  JsonArray press = hourly.containsKey("surface_pressure") ? hourly["surface_pressure"].as<JsonArray>() : JsonArray();

  int totalHours = times.size();
  if (totalHours <= 0) {
    s_lastError = "No hourly entries";
    Serial.println("[Weather] No hourly entries");
    return false;
  }

  // We'll sample every 6 hours starting from index 0, up to next 72 hours (3 days)
  const int HOURS_TO_COVER = 72;
  const int STEP = 6;
  int maxSamples = (HOURS_TO_COVER / STEP);
  // free previous cache
  if (s_days) { delete[] s_days; s_days = nullptr; s_daysCount = 0; }
  s_days = new WeatherDay[maxSamples];
  int samples = 0;
  for (int offset = 0; offset < HOURS_TO_COVER && (offset < totalHours); offset += STEP) {
    int idx = offset; // index into hourly arrays
    if (idx >= totalHours) break;
    String tISO = String(times[idx].as<const char*>());
    double temp = temps[idx].as<double>();
    int wc = 0;
    if (idx < (int)codes.size()) wc = codes[idx].as<int>();
    double hum = NAN;
    if (!hums.isNull() && idx < (int)hums.size()) hum = hums[idx].as<double>();
    double pr = NAN;
    if (!press.isNull() && idx < (int)press.size()) pr = press[idx].as<double>();

    // convert pressure to hPa if necessary:
    // - Open-Meteo surface_pressure typically returns hPa. But if a value looks like Pa (>2000),
    //   convert by dividing by 100.
    if (!isnan(pr)) {
      if (pr > 2000.0) pr = pr / 100.0;
    }

    // convert ISO "YYYY-MM-DDThh:mm" or "YYYY-MM-DD hh:mm" to "DD-MM HH:MM"
    String hhmm = "00:00";
    int posT = tISO.indexOf('T');
    if (posT < 0) posT = tISO.indexOf(' ');
    if (posT >= 0) hhmm = tISO.substring(posT+1, posT+6);
    int y = tISO.substring(0,4).toInt();
    int m = tISO.substring(5,7).toInt();
    int d = tISO.substring(8,10).toInt();

    char dbuf[20];
    snprintf(dbuf, sizeof(dbuf), "%02d-%02d %s", d, m, hhmm.c_str());
    s_days[samples].date = String(dbuf);
    s_days[samples].temp_min = (float)temp;
    s_days[samples].temp_max = (float)temp;
    s_days[samples].humidity = isnan(hum) ? 0.0f : (float)hum; // percent
    s_days[samples].pressure = isnan(pr) ? 0.0f : (float)pr;   // hPa
    s_days[samples].desc = mapWeatherCodeOpenMeteo(wc);
    samples++;
    if (samples >= maxSamples) break;
  }

  if (samples == 0) {
    s_lastError = "No samples parsed";
    Serial.println("[Weather] No samples parsed from OpenMeteo");
    // free memory
    delete[] s_days;
    s_days = nullptr;
    s_daysCount = 0;
    s_hasData = false;
    return false;
  }

  s_daysCount = samples;
  s_hasData = true;
  s_lastError = "";
  Serial.print("[Weather] OpenMeteo fetch OK, samples=");
  Serial.println(s_daysCount);
  return true;
}

bool weather_fetch() {
#if USE_OPENMETEO
  return weather_fetch_open_meteo();
#else
  s_lastError = "OpenMeteo disabled in config.h";
  Serial.println("[Weather] No provider enabled");
  return false;
#endif
}

bool weather_hasData() { return s_hasData; }
int weather_daysCount() { return s_daysCount; }

void weather_getDay(int idx, WeatherDay &out) {
  if (!s_hasData || idx < 0 || idx >= s_daysCount) {
    out.date = String("--");
    out.temp_min = 0.0f;
    out.temp_max = 0.0f;
    out.humidity = 0.0f;
    out.pressure = 0.0f;
    out.desc = String("N/A");
    return;
  }
  out = s_days[idx];
}

/* Debug helper (no API key printed) */
bool weather_debug_dumpAndFetch() {
  Preferences cp;
  cp.begin(PREF_NS, true);
  String latS = cp.getString(PREF_KEY_LAT, "");
  String lonS = cp.getString(PREF_KEY_LON, "");
  cp.end();
  double lat = s_lat, lon = s_lon;
  if (latS.length() && lonS.length()) { lat = latS.toDouble(); lon = lonS.toDouble(); }
  String fullUrl = String("https://api.open-meteo.com/v1/forecast?latitude=")
    + String(lat, 6) + "&longitude=" + String(lon, 6)
    + "&hourly=temperature_2m,weathercode,relativehumidity_2m,surface_pressure&forecast_days=3&timezone=auto";
  Serial.print("[Debug] Request URL: ");
  Serial.println(fullUrl);
  bool res = weather_fetch();
  return res;
}