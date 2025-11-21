#include "menu_manager.h"
#include "ui.h"
#include "text_strings.h"
#include "config.h"
#include "time_manager.h"
#include "modem_manager.h"
#include "weather_manager.h"
#include "provisioning_ui.h"
#include "sms_handler.h"
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

extern LiquidCrystal_I2C lcd;

// =====================================================================
// MENU ITEMS
// =====================================================================
static MenuItem root;
static MenuItem* currentItem = nullptr;

// Forward declarations
static void menuShowStatus();
static void menuShowTime();
static void menuShowMeasurements();
static void menuShowCalibration();
static void menuShowSDInfo();
static void menuSetLanguage();
static void menuCalTare();
static void menuCalCalibrate();
static void menuCalRaw();
static void menuCalSave();
static void menuShowConnectivity();
static void menuShowWeather();
static void menuShowProvision(); // PROVISION menu

// MAIN MENU ITEMS
static MenuItem m_status;
static MenuItem m_time;
static MenuItem m_measure;
static MenuItem m_weather;
static MenuItem m_connectivity;
static MenuItem m_provision;
static MenuItem m_calibration;
static MenuItem m_language;
static MenuItem m_sdinfo;
static MenuItem m_back;

// CALIB SUBMENU
static MenuItem cal_root;
static MenuItem m_cal_tare;
static MenuItem m_cal_cal;
static MenuItem m_cal_raw;
static MenuItem m_cal_save;
static MenuItem m_cal_back;

// Dummy calibration placeholders
static float cal_knownWeightKg = 1.0f;
static float cal_factor        = 800.0f;
static long  cal_offset        = 0;
static long  cal_rawReading    = 750000;

// =====================================================================
// INIT
// =====================================================================
void menuInit() {
  // ORDER:
  // STATUS -> TIME -> MEASUREMENTS -> WEATHER -> CONNECTIVITY -> PROVISION -> CALIBRATION -> LANGUAGE -> SD INFO -> BACK

  m_status       = { TXT_STATUS,       menuShowStatus,       &m_time,        nullptr,       &root,     nullptr };
  m_time         = { TXT_TIME,         menuShowTime,         &m_measure,     &m_status,     &root,     nullptr };
  m_measure      = { TXT_MEASUREMENTS, menuShowMeasurements, &m_weather,     &m_time,       &root,     nullptr };
  m_weather      = { TXT_WEATHER,      menuShowWeather,      &m_connectivity,&m_measure,    &root,     nullptr };
  m_connectivity = { TXT_CONNECTIVITY, menuShowConnectivity, &m_provision,   &m_weather,    &root,     nullptr };
  m_provision    = { TXT_PROVISION,    menuShowProvision,    &m_calibration, &m_connectivity,&root,    nullptr };
  m_calibration  = { TXT_CALIBRATION,  menuShowCalibration,  &m_language,    &m_provision,  &root,     &cal_root };
  m_language     = { TXT_LANGUAGE,     menuSetLanguage,      &m_sdinfo,      &m_calibration,&root,     nullptr };
  m_sdinfo       = { TXT_SD_INFO,      menuShowSDInfo,       &m_back,        &m_language,   &root,     nullptr };
  m_back         = { TXT_BACK,         nullptr,              nullptr,        &m_sdinfo,     &root,     nullptr };

  root.text  = TXT_NONE;
  root.child = &m_status;

  // CALIBRATION SUBMENU
  m_cal_tare = { TXT_TARE,            menuCalTare,          &m_cal_cal,  nullptr,   &cal_root, nullptr };
  m_cal_cal  = { TXT_CALIBRATE_KNOWN, menuCalCalibrate,     &m_cal_raw,  &m_cal_tare,&cal_root, nullptr };
  m_cal_raw  = { TXT_RAW_VALUE,       menuCalRaw,           &m_cal_save, &m_cal_cal,&cal_root, nullptr };
  m_cal_save = { TXT_SAVE_FACTOR,     menuCalSave,          &m_cal_back, &m_cal_raw,&cal_root, nullptr };
  m_cal_back = { TXT_BACK,            nullptr,              nullptr,     &m_cal_save,&root,    nullptr };

  cal_root   = { TXT_CALIBRATION,     nullptr,              &m_cal_tare, nullptr,   &root,     nullptr };

  currentItem = &m_status;
}

// =====================================================================
// DRAW MAIN MENU (scroll 3 lines, cyclic)
// =====================================================================
void menuDraw() {
  uiClear();

  MenuItem* list[] = {
    &m_status,
    &m_time,
    &m_measure,
    &m_weather,
    &m_connectivity,
    &m_provision,
    &m_calibration,
    &m_language,
    &m_sdinfo,
    &m_back
  };

  const int MENU_COUNT = sizeof(list) / sizeof(list[0]);

  int selectedIndex = 0;
  for (int i = 0; i < MENU_COUNT; i++) {
    if (list[i] == currentItem) {
      selectedIndex = i;
      break;
    }
  }

  static int scroll = 0;
  if (selectedIndex < scroll)         scroll = selectedIndex;
  if (selectedIndex > scroll + 3)     scroll = selectedIndex - 3;

  for (int line = 0; line < 4; line++) {
    int idx = scroll + line;
    if (idx >= MENU_COUNT) break;

    TextId id = list[idx]->text;
    const char* label = (currentLanguage == LANG_EN) ? getTextEN(id) : getTextGR(id);

    uiPrint(0, line, (list[idx] == currentItem ? ">" : " "));

    if (currentLanguage == LANG_EN)
      uiPrint(1, line, label);
    else
      lcdPrintGreek(label, 1, line);
  }
}

// =====================================================================
// BUTTON HANDLING (cyclic up/down)
// =====================================================================
void menuUpdate() {
  Button b = getButton();
  if (b == BTN_NONE) return;

  MenuItem* parent = currentItem->parent;
  if (!parent) parent = &root;

  MenuItem* first = parent->child;
  MenuItem* last  = first;
  while (last->next) last = last->next;

  if (b == BTN_UP_PRESSED) {
    if (currentItem->prev)
      currentItem = currentItem->prev;
    else
      currentItem = last;
    menuDraw();
    return;
  }

  if (b == BTN_DOWN_PRESSED) {
    if (currentItem->next)
      currentItem = currentItem->next;
    else
      currentItem = first;
    menuDraw();
    return;
  }

  if (b == BTN_BACK_PRESSED) {
    if (currentItem->parent) {
      currentItem = currentItem->parent;
      menuDraw();
    }
    return;
  }

  if (b == BTN_SELECT_PRESSED) {
    if (currentItem->action) {
      currentItem->action();
      return;
    }
    if (currentItem->child) {
      currentItem = currentItem->child;
      menuDraw();
      return;
    }
  }
}

// =====================================================================
// STATUS SCREEN
// =====================================================================
static void menuShowStatus() {
  uiClear();

  unsigned long lastUpdate = 0;
  String oldDateTime = "";
  float  oldWeight = -999;
  float  oldBattV  = -999;
  int    oldBattP  = -1;

  while (true) {
    timeManager_update();
    unsigned long now = millis();

    if (now - lastUpdate >= 1000) {
      lastUpdate = now;

      String dt;
      if (timeManager_isTimeValid()) {
        dt = timeManager_getDate() + " " + timeManager_getTime();
      } else {
        dt = "01-01-1970  00:00:00";
      }

      if (dt != oldDateTime) {
        if (currentLanguage == LANG_EN)
          uiPrint(0, 0, dt.c_str());
        else
          lcdPrintGreek(dt.c_str(), 0, 0);
        oldDateTime = dt;
      }

      float w = test_weight;
      char line[21];

      if (fabs(w - oldWeight) > 0.01f) {
        if (currentLanguage == LANG_EN)
          snprintf(line, 21, "WEIGHT: %5.1f kg   ", w);
        else
          snprintf(line, 21, "\u0392\u0391\u03a1\u039f\u03a3: %5.1fkg     ", w);

        if (currentLanguage == LANG_EN)
          uiPrint(0, 1, line);
        else
          lcdPrintGreek(line, 0, 1);

        oldWeight = w;
      }

      float bv = test_batt_voltage;
      int   bp = test_batt_percent;

      if (fabs(bv - oldBattV) > 0.01f || bp != oldBattP) {
        if (currentLanguage == LANG_EN)
          snprintf(line, 21, "BATTERY: %.2fV %3d%% ", bv, bp);
        else
          snprintf(line, 21, "\u039c\u03a0\u0391\u03a4\u0391\u03a1\u0399\u0391:%.2fV %3d%% ", bv, bp);

        if (currentLanguage == LANG_EN)
          uiPrint(0, 2, line);
        else
          lcdPrintGreek(line, 0, 2);

        oldBattV = bv;
        oldBattP = bp;
      }

      if (currentLanguage == LANG_EN)
        uiPrint(0, 3, getTextEN(TXT_BACK_SMALL));
      else
        lcdPrintGreek(getTextGR(TXT_BACK_SMALL), 0, 3);
    }

    Button b = getButton();
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) {
      menuDraw();
      return;
    }

    delay(20);
  }
}

// =====================================================================
// TIME SCREEN
// =====================================================================
static void menuShowTime() {
  uiClear();

  unsigned long lastUpdate = 0;
  String oldDate = "";
  String oldTime = "";
  TimeSource oldSrc = TSRC_NONE;

  while (true) {
    unsigned long now = millis();

    if (now - lastUpdate >= 1000) {
      lastUpdate = now;

      String d = timeManager_getDate();
      String t = timeManager_getTime();
      TimeSource src = timeManager_getSource();

      const char* srcName =
        (src == TSRC_WIFI) ? "WIFI" :
        (src == TSRC_LTE)  ? "LTE"  : "NONE";

      if (d != oldDate) {
        if (currentLanguage == LANG_EN)
          uiPrint(0, 0, (String("DATE: ") + d).c_str());
        else {
          char line[21];
          snprintf(line, 21, "\u0397\u039c/\u039d\u0399\u0391: %s", d.c_str());
          lcdPrintGreek(line, 0, 0);
        }
        oldDate = d;
      }

      if (t != oldTime) {
        if (currentLanguage == LANG_EN)
          uiPrint(0, 1, (String("TIME: ") + t).c_str());
        else {
          char line[21];
          snprintf(line, 21, "\u03a9\u03a1\u0391:    %s", t.c_str());
          lcdPrintGreek(line, 0, 1);
        }
        oldTime = t;
      }

      if (src != oldSrc) {
        if (currentLanguage == LANG_EN)
          uiPrint(0, 2, (String("SRC:  ") + srcName).c_str());
        else {
          char line[21];
          snprintf(line, 21, "\u03a0\u0397\u0393\u0397:   %s", srcName);
          lcdPrintGreek(line, 0, 2);
        }
        oldSrc = src;
      }

      if (currentLanguage == LANG_EN)
        uiPrint(0, 3, getTextEN(TXT_BACK_SMALL));
      else
        lcdPrintGreek(getTextGR(TXT_BACK_SMALL), 0, 3);
    }

    Button b = getButton();
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) {
      menuDraw();
      return;
    }

    delay(20);
  }
}

// =====================================================================
// MEASUREMENTS
// =====================================================================
static void menuShowMeasurements() {
  int page = 0;
  int lastPage = -1;
  const int maxPage = 2;
  char line[21];

  while (true) {
    if (page != lastPage) {
      uiClear();

      if (currentLanguage == LANG_EN) {
        uiPrint(0, 0, getTextEN(TXT_MEASUREMENTS));

        if (page == 0) {
          snprintf(line, 21, "WEIGHT: %5.1f kg  ", test_weight);
          uiPrint(0, 1, line);

          snprintf(line, 21, "T_INT:  %4.1fC     ", test_temp_int);
          uiPrint(0, 2, line);

          snprintf(line, 21, "H_INT:  %3.0f%%     ", test_hum_int);
          uiPrint(0, 3, line);
        } else if (page == 1) {
          snprintf(line, 21, "T_EXT:  %4.1fC     ", test_temp_ext);
          uiPrint(0, 1, line);

          snprintf(line, 21, "H_EXT:  %3.0f%%     ", test_hum_ext);
          uiPrint(0, 2, line);

          snprintf(line, 21, "PRESS: %4.0fhPa    ", test_pressure);
          uiPrint(0, 3, line);
        } else {
          snprintf(line, 21, "ACC: X%.2f Y%.2f   ", test_acc_x, test_acc_y);
          uiPrint(0, 1, line);

          snprintf(line, 21, "Z: %.2f            ", test_acc_z);
          uiPrint(0, 2, line);

          snprintf(line, 21, "BAT: %.2fV %3d%%    ", test_batt_voltage, test_batt_percent);
          uiPrint(0, 3, line);
        }
      } else {
        lcdPrintGreek(getTextGR(TXT_MEASUREMENTS), 0, 0);

        if (page == 0) {
          snprintf(line, 21, "\u0392\u0391\u03a1\u039f\u03a3: %5.1fkg     ", test_weight);
          lcdPrintGreek(line, 0, 1);

          snprintf(line, 21, "\u0398\u0395\u03a1\u039c. \u0395\u03a3\u03a9: %4.1fC  ", test_temp_int);
          lcdPrintGreek(line, 0, 2);

          snprintf(line, 21, "\u03a5\u0393\u03a1. \u0395\u03a3\u03a9: %3.0f%%   ", test_hum_int);
          lcdPrintGreek(line, 0, 3);
        } else if (page == 1) {
          snprintf(line, 21, "\u0398\u0395\u03a1\u039c. \u0395\u039a\u03a9: %4.1fC  ", test_temp_ext);
          lcdPrintGreek(line, 0, 1);

          snprintf(line, 21, "\u03a5\u0393\u03a1. \u0395\u039a\u03a9: %3.0f%%   ", test_hum_ext);
          lcdPrintGreek(line, 0, 2);

          snprintf(line, 21, "\u0391\u03a4\u039c. \u03a0\u0399\u0395\u03a3\u0397:%4.0fhPa", test_pressure);
          lcdPrintGreek(line, 0, 3);
        } else {
          snprintf(line, 21, "\u0395\u03a0\u0399\u03a4:X%.2f Y%.2f    ", test_acc_x, test_acc_y);
          lcdPrintGreek(line, 0, 1);

          snprintf(line, 21, "Z:%.2f             ", test_acc_z);
          lcdPrintGreek(line, 0, 2);

          snprintf(line, 21, "\u039c\u03a0\u0391\u03a4:%.2fV %3d%%    ", test_batt_voltage, test_batt_percent);
          lcdPrintGreek(line, 0, 3);
        }
      }

      lastPage = page;
    }

    Button b = getButton();
    if (b == BTN_UP_PRESSED) {
      page--;
      if (page < 0) page = maxPage;
    }
    if (b == BTN_DOWN_PRESSED) {
      page++;
      if (page > maxPage) page = 0;
    }
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) {
      menuDraw();
      return;
    }

    delay(80);
  }
}

// =====================================================================
// SD CARD INFO
// =====================================================================
static void menuShowSDInfo() {
  uiClear();
  bool ok = SD.begin(SD_CS);

  if (currentLanguage == LANG_EN) {
    uiPrint(0, 0, getTextEN(TXT_SD_CARD_INFO));
    uiPrint(0, 1, ok ? getTextEN(TXT_SD_OK) : getTextEN(TXT_NO_CARD));
    uiPrint(0, 3, getTextEN(TXT_BACK_SMALL));
  } else {
    lcdPrintGreek(getTextGR(TXT_SD_CARD_INFO), 0, 0);
    lcdPrintGreek(ok ? getTextGR(TXT_SD_OK) : getTextGR(TXT_NO_CARD), 0, 1);
    lcdPrintGreek(getTextGR(TXT_BACK_SMALL), 0, 3);
  }

  while (true) {
    Button b = getButton();
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) {
      menuDraw();
      return;
    }
    delay(50);
  }
}

// =====================================================================
// LANGUAGE
// =====================================================================
static void menuSetLanguage() {
  currentLanguage = (currentLanguage == LANG_EN ? LANG_GR : LANG_EN);

  uiClear();
  if (currentLanguage == LANG_EN)
    uiPrint(0, 0, getTextEN(TXT_LANGUAGE_EN));
  else
    lcdPrintGreek(getTextGR(TXT_LANGUAGE_GR),0,0);

  delay(500);
  menuDraw();
}

// =====================================================================
// CALIBRATION
// =====================================================================
static void menuShowCalibration() {
  uiClear();

  if (currentLanguage == LANG_EN) {
    uiPrint(0, 0, "1) TARE              ");
    uiPrint(0, 1, "2) CALIBRATION       ");
    uiPrint(0, 2, "3) RAW VALUE         ");
    uiPrint(0, 3, getTextEN(TXT_BACK_SMALL));
  } else {
    lcdPrintGreek("1) \u039c\u0397\u0394\u0395\u039d\u0399\u03a3\u039c\u039f\u03a3       ", 0, 0);
    lcdPrintGreek("2) \u0392\u0391\u0398\u039c\u039f\u039d\u039f\u039c\u0397\u03a3\u0397      ", 0, 1);
    lcdPrintGreek("3) RAW \u03a4\u0399\u039c\u0397         ", 0, 2);
    lcdPrintGreek(getTextGR(TXT_BACK_SMALL), 0, 3);
  }
}

static void menuCalTare() {
  uiClear();
  if (currentLanguage == LANG_EN) uiPrint(0, 0, getTextEN(TXT_TARE_DONE));
  else lcdPrintGreek(getTextGR(TXT_TARE_DONE),0,0);
  delay(800);
  menuDraw();
}

static void menuCalCalibrate() {
  uiClear();
  if (currentLanguage == LANG_EN) uiPrint(0, 0, getTextEN(TXT_CALIBRATION_DONE));
  else lcdPrintGreek(getTextGR(TXT_CALIBRATION_DONE),0,0);
  delay(800);
  menuDraw();
}

static void menuCalRaw() {
  uiClear();
  char line[21];
  snprintf(line, 21, "RAW: %ld        ", cal_rawReading);
  uiPrint(0, 1, line);
  uiPrint(0, 3, getTextEN(TXT_BACK_SMALL));

  while (true) {
    Button b = getButton();
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) {
      menuDraw();
      return;
    }
    delay(60);
  }
}

static void menuCalSave() {
  uiClear();
  if (currentLanguage == LANG_EN) uiPrint(0, 0, getTextEN(TXT_FACTOR_SAVED));
  else lcdPrintGreek(getTextGR(TXT_FACTOR_SAVED),0,0);
  delay(800);
  menuDraw();
}

// =====================================================================
// CONNECTIVITY
// =====================================================================
static void menuShowConnectivity() {
  uiClear();

  while (true) {
    bool wifiOK = (WiFi.status() == WL_CONNECTED);
    bool lteOK  = modem_isNetworkRegistered();

    char line[21];

    if (wifiOK) {
      int32_t rssi = WiFi.RSSI();
      uiPrint(0, 0, getTextEN(TXT_WIFI_CONNECTED));

      snprintf(line, 21, "%s %s", getTextEN(TXT_SSID), WiFi.SSID().c_str());
      uiPrint(0, 1, line);

      snprintf(line, 21, "%s %ddBm", getTextEN(TXT_RSSI), rssi);
      uiPrint(0, 2, line);
    } else if (lteOK) {
      int16_t rssi = modem_getRSSI();

      uiPrint(0, 0, getTextEN(TXT_LTE_REGISTERED));
      snprintf(line, 21, "%s %ddBm", getTextEN(TXT_RSSI), rssi);
      uiPrint(0, 1, line);
      uiPrint(0, 2, "MODE: LTE");
    } else {
      uiPrint(0, 0, getTextEN(TXT_NO_CONNECTIVITY));
      uiPrint(0, 1, "                   ");
      uiPrint(0, 2, "                   ");
    }

    uiPrint(0, 3, getTextEN(TXT_BACK_SMALL));

    Button b = getButton();
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) {
      menuDraw();
      return;
    }

    delay(200);
  }
}

// =====================================================================
// WEATHER MENU (NEW)
// =====================================================================
// Replace only the menuShowWeather() function in menu_manager.cpp with this version.
// Displays a 2s location card where:
//  - Row 0: header "WEATHER=====>SEL====>"
//  - Row 1: LAT: xx.xx, LON: yy.yy
//  - Row 2: City, CC (if available)
//  - Row 3: "< BACK"
// Then proceeds to fetch and show the 12 six‑hour forecast samples as before.

static void menuShowWeather() {
  uiClear();

  // Read stored place name/country and coords from Preferences
  Preferences p;
  p.begin("beehive", true);
  String placeName = p.getString("loc_name", "");
  String country = p.getString("loc_country", "");
  String latS = p.getString("owm_lat", "");
  String lonS = p.getString("owm_lon", "");
  p.end();

  // Prepare numeric lat/lon for display (2 decimal places)
  double lat = DEFAULT_LAT;
  double lon = DEFAULT_LON;
  if (latS.length() && lonS.length()) {
    lat = latS.toDouble();
    lon = lonS.toDouble();
  }

  char line0[21];
  char line1[21];
  char line2[21];
  char line3[21];

  // Row 0: static header
  snprintf(line0, sizeof(line0), "%-20s", "WEATHER=====>SEL==>");

  // Row 1: LAT/LON with 2 decimals
  snprintf(line1, sizeof(line1), "LAT:%6.2f LON:%6.2f", lat, lon);

  // Row 2: place name, country if available
  if (placeName.length() > 0) {
    if (country.length() > 0) {
      String pc = placeName + ", " + country;
      // truncate/pad to 20 chars
      snprintf(line2, sizeof(line2), "%-20s", pc.c_str());
    } else {
      snprintf(line2, sizeof(line2), "%-20s", placeName.c_str());
    }
  } else {
    snprintf(line2, sizeof(line2), "%-20s", " ");
  }

  // Row 3: back label
  snprintf(line3, sizeof(line3), "%-20s", getTextEN(TXT_BACK_SMALL));

  // Display the location card for 2 seconds
  uiClear();
  if (currentLanguage == LANG_EN) {
    uiPrint(0, 0, line0);
    uiPrint(0, 1, line1);
    uiPrint(0, 2, line2);
    uiPrint(0, 3, line3);
  } else {
    // For Greek, show rows with greek-print helper where appropriate.
    lcdPrintGreek(line0, 0, 0);
    lcdPrintGreek(line1, 0, 1);
    lcdPrintGreek(line2, 0, 2);
    lcdPrintGreek(getTextGR(TXT_BACK_SMALL), 0, 3);
  }
  delay(2000);

  // Now proceed to the normal weather fetch and paging UI
  int page = 0;
  int lastPage = -1;
  WeatherDay wd;

  // Show fetching indicator briefly
  uiClear();
  if (currentLanguage == LANG_EN)
    uiPrint(0,0,getTextEN(TXT_FETCHING_WEATHER));
  else
    lcdPrintGreek(getTextGR(TXT_FETCHING_WEATHER),0,0);

  // Blocking fetch (brief)
  weather_fetch();

  while (true) {
    int total = weather_daysCount();
    int maxPage = (total > 0) ? (total - 1) : 0;

    if (page != lastPage) {
      uiClear();
      if (!weather_hasData()) {
        if (currentLanguage == LANG_EN) uiPrint(0,0,getTextEN(TXT_WEATHER_NO_DATA));
        else lcdPrintGreek(getTextGR(TXT_WEATHER_NO_DATA), 0, 0);
      } else {
        // clamp page
        if (page < 0) page = 0;
        if (page > maxPage) page = maxPage;

        weather_getDay(page, wd);
        char line[21];

        // Line 0: date/time
        if (currentLanguage == LANG_EN) {
          snprintf(line,21,"%s                ", wd.date.c_str());
          uiPrint(0,0,line);
          // Line 1: description (trim/pad)
          snprintf(line,21,"%-20s", wd.desc.c_str());
          uiPrint(0,1,line);
          // Line 2: temperature and humidity
          snprintf(line,21,"T:%5.1fC H:%3.0f%%", wd.temp_min, wd.humidity);
          uiPrint(0,2,line);
          // Line 3: pressure and back label
          snprintf(line,21,"P:%5.0fhPa %s", wd.pressure, getTextEN(TXT_BACK_SMALL));
          uiPrint(0,3,line);
        } else {
          // Greek: show same patterns using lcdPrintGreek for strings
          snprintf(line,21,"%s                ", wd.date.c_str());
          lcdPrintGreek(line,0,0);
          lcdPrintGreek(wd.desc.c_str(),0,1);
          snprintf(line,21,"T:%5.1fC H:%3.0f%%", wd.temp_min, wd.humidity);
          lcdPrintGreek(line,0,2);
          snprintf(line,21,"P:%5.0fhPa %s", wd.pressure, getTextEN(TXT_BACK_SMALL));
          lcdPrintGreek(line,0,3);
        }
      }
      lastPage = page;
    }

    Button b = getButton();
    if (b == BTN_UP_PRESSED) {
      page--;
      if (page < 0) page = maxPage;
    }
    if (b == BTN_DOWN_PRESSED) {
      page++;
      if (page > maxPage) page = 0;
    }
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) {
      menuDraw();
      return;
    }
    delay(80);
  }
}
// -----------------------------------------------------------------------------
// PROVISION MENU (updated: API key entry removed)
// -----------------------------------------------------------------------------
static void menuShowProvision() {
  uiClear();
  if (currentLanguage == LANG_EN) {
    uiPrint(0,0,getTextEN(TXT_PROVISION));
    uiPrint(0,1,"1) Geocode City      ");
    uiPrint(0,2,"                    ");
    uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
  } else {
    lcdPrintGreek(getTextGR(TXT_PROVISION),0,0);
    lcdPrintGreek("1) \u03a3\u0395\u0391 \u0393\u0395\u039f",0,1); // "1) SEA GEO" (approx)
    lcdPrintGreek("                    ",0,2);
    lcdPrintGreek(getTextGR(TXT_BACK_SMALL),0,3);
  }

  while (true) {
    Button b = getButton();
    if (b == BTN_SELECT_PRESSED) {
      // Only option: enter City/Country -> geocode
      provisioning_ui_enterCityCountry();
      menuDraw();
      return;
    } else if (b == BTN_BACK_PRESSED) {
      menuDraw();
      return;
    }
    delay(80);
  }
}