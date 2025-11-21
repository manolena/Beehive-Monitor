#include "time_manager.h"
#include "modem_manager.h"
#include "config.h"
#include <WiFi.h>
#include <time.h>

// ---------------------------------------------------------
// INTERNAL STATE
// ---------------------------------------------------------
enum TimeState {
  TS_IDLE,
  TS_LTE_CHECK,
  TS_WIFI_SCAN,
  TS_WIFI_CONNECTING,
  TS_NTP_REQUEST,
  TS_DONE,
  TS_FAIL
};

static TimeState    state       = TS_IDLE;
static unsigned long last_query = 0;
static int          attempt     = 0;

static bool        time_valid   = false;
static TimeSource  time_source  = TSRC_NONE;

// ---------------------------------------------------------
// WIFI HOTSPOTS (from your previous working setup)
// ---------------------------------------------------------
static const char* hotspot_ssids[] = {
  "COSMOTE-32bssa",
  "Redmi Note 13",
  nullptr
};

static const char* hotspot_pass[] = {
  "vudvvc5x97s4afpk",
  "nen57asz5g44sh2",
  nullptr
};

// ---------------------------------------------------------
// INIT
// ---------------------------------------------------------
void timeManager_init() {
  // Greece: GMT+2, DST +1
  configTime(2 * 3600, 3600, "pool.ntp.org", "time.google.com");

  state       = TS_LTE_CHECK;
  last_query  = 0;
  attempt     = 0;
  time_valid  = false;
  time_source = TSRC_NONE;
}

// ---------------------------------------------------------
// WIFI HELPER
// ---------------------------------------------------------
static bool tryConnectToWifi() {
  int n = WiFi.scanNetworks();
  if (n <= 0) return false;

  for (int i = 0; hotspot_ssids[i] != nullptr; i++) {
    const char* target = hotspot_ssids[i];
    for (int j = 0; j < n; j++) {
      if (WiFi.SSID(j) == target) {
        WiFi.begin(target, hotspot_pass[i]);
        return true;
      }
    }
  }
  return false;
}

// ---------------------------------------------------------
// UPDATE
// ---------------------------------------------------------
void timeManager_update() {
  if (time_valid) return;

  unsigned long now = millis();

  switch (state) {
    case TS_LTE_CHECK:
    {
      if (now - last_query < 3000) return;
      last_query = now;

      TinyGsm& modem = modem_get();

      modem.sendAT("+CCLK?");
      String resp = modem.stream.readString();

      if (resp.indexOf("+CCLK:") >= 0) {
        int y, M, d, h, m, s, tz;
        if (sscanf(resp.c_str(),
                   "*+CCLK: \"%d/%d/%d,%d:%d:%d+%d",
                   &y, &M, &d, &h, &m, &s, &tz) == 7) {
          struct tm t;
          t.tm_year = 2000 + y - 1900;
          t.tm_mon  = M - 1;
          t.tm_mday = d;
          t.tm_hour = h;
          t.tm_min  = m;
          t.tm_sec  = s;

          time_t tt = mktime(&t);
          struct timeval tv = { tt, 0 };
          settimeofday(&tv, nullptr);

          time_valid  = true;
          time_source = TSRC_LTE;
          state       = TS_DONE;
          break;
        }
      }

      // If LTE time failed → fallback to WiFi NTP
      state = TS_WIFI_SCAN;
      break;
    }

    case TS_WIFI_SCAN:
      if (now - last_query < 5000) return;
      last_query = now;

      WiFi.mode(WIFI_STA);
      WiFi.disconnect(false);

      if (tryConnectToWifi())
        state = TS_WIFI_CONNECTING;
      else
        state = TS_FAIL;
      break;

    case TS_WIFI_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        time_source = TSRC_WIFI;
        state       = TS_NTP_REQUEST;
        last_query  = now;
      } else if (now - last_query > 8000) {
        state = TS_FAIL;
      }
      break;

    case TS_NTP_REQUEST:
    {
      time_t t = time(nullptr);
      if (t > 100000) {
        time_valid = true;
        state      = TS_DONE;
      } else if (now - last_query > 5000) {
        configTime(2 * 3600, 3600, "pool.ntp.org", "time.google.com");
        last_query = now;
      }
      break;
    }

    case TS_DONE:
      time_valid = true;
      break;

    case TS_FAIL:
      // no LTE, no WiFi time
      break;

    default:
      break;
  }
}

// ---------------------------------------------------------
// ACCESSORS
// ---------------------------------------------------------
bool timeManager_isTimeValid() {
  return time_valid;
}

String timeManager_getDate() {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);

  char buf[16];
  snprintf(buf, sizeof(buf), "%02d-%02d-%04d",
           t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
  return String(buf);
}

String timeManager_getTime() {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);

  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
           t.tm_hour, t.tm_min, t.tm_sec);
  return String(buf);
}

TimeSource timeManager_getSource() {
  return time_source;
}
