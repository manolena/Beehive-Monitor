// provisioning_ui.cpp (modified): long-press SELECT in country entry saves immediately.
// City entry unchanged from previous trimmed version; country entry now supports:
//  - short SELECT: advance/save (same as before)
//  - long SELECT (hold >= SELECT_SAVE_HOLD_MS): save immediately and return

#include "provisioning_ui.h"
#include "ui.h"
#include "weather_manager.h"
#include "text_strings.h"
#include "menu_manager.h"
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <WiFi.h>

static const char charset[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789"
  "-_.:/ "; // allow a few extras and space

static const int MAX_CITY = 20;
static const unsigned long CURSOR_BLINK_MS = 500;
static const unsigned long REPEAT_INITIAL_MS = 400;
static const unsigned long REPEAT_FAST_MS = 100;
static const unsigned long REPEAT_ACCEL_MS = 700; // time before fast repeat
static const unsigned long SELECT_SAVE_HOLD_MS = 800; // hold SELECT this long to save
static const int LCD_COLS = 20;

// UTF-8-aware truncate/pad (best-effort)
static void utf8_truncate_pad(const char* src, char* dst, int maxChars) {
  int outVisual = 0;
  int i = 0, o = 0;
  int len = strlen(src);
  while (i < len && outVisual < maxChars) {
    unsigned char c = (unsigned char)src[i];
    if ((c & 0x80) == 0) { dst[o++] = src[i++]; outVisual++; }
    else if ((c & 0xE0) == 0xC0) {
      if (i + 1 < len && outVisual + 1 <= maxChars) { dst[o++] = src[i++]; dst[o++] = src[i++]; outVisual++; } else break;
    } else if ((c & 0xF0) == 0xE0) {
      if (i + 2 < len && outVisual + 1 <= maxChars) { dst[o++] = src[i++]; dst[o++] = src[i++]; dst[o++] = src[i++]; outVisual++; } else break;
    } else { dst[o++] = src[i++]; outVisual++; }
  }
  while (outVisual < maxChars) { dst[o++] = ' '; outVisual++; }
  dst[o] = 0;
}

static void enPrintFixed(uint8_t col, uint8_t row, const char* msg) {
  char buf[21];
  int i;
  for (i = 0; i < LCD_COLS; ++i) buf[i] = msg[i] ? msg[i] : ' ';
  buf[LCD_COLS] = 0;
  uiPrint(col, row, buf);
}

static void grPrintFixed(uint8_t col, uint8_t row, const char* msg) {
  char tmp[128];
  utf8_truncate_pad(msg, tmp, LCD_COLS);
  lcdPrintGreek(tmp, col, row);
}

static void uiShowPromptId(TextId id) {
  uiClear();
  if (currentLanguage == LANG_EN) enPrintFixed(0,0, getTextEN(id));
  else grPrintFixed(0,0, getTextGR(id));
}

// ---------------------------
// City entry + country (country default = two spaces, SEL while "  " saves city-only)
// ---------------------------
void provisioning_ui_enterCityCountry() {
  static char city[MAX_CITY+1];
  memset(city, 0, sizeof(city));
  char country[4] = {' ', ' ', 0}; // default two spaces: means "no country"
  int pos = 0, used = 0;
  unsigned long lastBlink = millis(); bool blinkOn = true;
  uiShowPromptId(TXT_ENTER_CITY);

  auto drawCity = [&]() {
    char line[21];
    for (int i = 0; i < 20; ++i) line[i] = (i < used) ? city[i] : ' ';
    line[20] = 0;
    if (currentLanguage == LANG_EN) enPrintFixed(0,1,line); else grPrintFixed(0,1,line);

    char cursorLine[21];
    for (int i = 0; i < 20; ++i) cursorLine[i] = ' ';
    cursorLine[20] = 0;
    if (used > 0) {
      int caretPos = (pos < 20) ? pos : 19;
      cursorLine[caretPos] = blinkOn ? '^' : ' ';
    } else {
      cursorLine[0] = blinkOn ? '^' : ' ';
    }
    if (currentLanguage == LANG_EN) enPrintFixed(0,2,cursorLine); else grPrintFixed(0,2,cursorLine);

    if (currentLanguage == LANG_EN) enPrintFixed(0,3,"SEL:NEXT BK:CANCEL");
    else grPrintFixed(0,3,"SEL:ΕΠΟΜ BK:ΑΚΥΡ");
  };

  drawCity();

  unsigned long upHoldStart=0, downHoldStart=0, upLastAct=0, downLastAct=0;
  bool upPrev=false, downPrev=false, selPrev=false, backPrev=false;

  while (true) {
    unsigned long now = millis();
    if (now - lastBlink >= CURSOR_BLINK_MS) { lastBlink = now; blinkOn = !blinkOn; drawCity(); }

    bool upNow = (digitalRead(BTN_UP) == LOW);
    bool downNow = (digitalRead(BTN_DOWN) == LOW);
    bool selNow = (digitalRead(BTN_SELECT) == LOW);
    bool backNow = (digitalRead(BTN_BACK) == LOW);

    // UP (initial + hold)
    if (upNow && !upPrev) {
      if (used == 0) { city[0] = 'A'; used = 1; pos = 0; }
      else {
        char *p = strchr(charset, city[pos]);
        if (!p) city[pos] = 'A';
        else { int idx = (p - charset); idx = (idx + 1) % (int)strlen(charset); city[pos] = charset[idx]; }
      }
      upHoldStart = now; upLastAct = now; drawCity();
    } else if (upNow && upPrev) {
      unsigned long held = now - upHoldStart;
      unsigned long interval = (held >= REPEAT_ACCEL_MS) ? REPEAT_FAST_MS : REPEAT_INITIAL_MS;
      if (now - upLastAct >= interval) { upLastAct = now; char *p=strchr(charset, city[pos]); if(!p) city[pos]='A'; else { int idx=(p - charset); idx=(idx+1)%(int)strlen(charset); city[pos]=charset[idx]; } drawCity(); }
    }
    upPrev = upNow;

    // DOWN (initial + hold)
    if (downNow && !downPrev) {
      if (used == 0) { city[0] = 'Z'; used = 1; pos = 0; }
      else {
        char *p = strchr(charset, city[pos]);
        if (!p) city[pos] = 'Z';
        else { int idx = (p - charset); idx = (idx - 1 + (int)strlen(charset)) % (int)strlen(charset); city[pos] = charset[idx]; }
      }
      downHoldStart = now; downLastAct = now; drawCity();
    } else if (downNow && downPrev) {
      unsigned long held = now - downHoldStart;
      unsigned long interval = (held >= REPEAT_ACCEL_MS) ? REPEAT_FAST_MS : REPEAT_INITIAL_MS;
      if (now - downLastAct >= interval) { downLastAct = now; char *p=strchr(charset, city[pos]); if(!p) city[pos]='Z'; else { int idx=(p - charset); idx=(idx-1 + (int)strlen(charset))%(int)strlen(charset); city[pos]=charset[idx]; } drawCity(); }
    }
    downPrev = downNow;

    // SELECT
    if (selNow && !selPrev) {
      if (used < MAX_CITY) { used++; pos = used - 1; city[used] = 0; }
      else break;
      drawCity();
    }
    selPrev = selNow;

    // BACK
    if (backNow && !backPrev) {
      uiClear();
      if (currentLanguage == LANG_EN) enPrintFixed(0,0,getTextEN(TXT_CANCELLED));
      else grPrintFixed(0,0,getTextGR(TXT_CANCELLED));
      delay(400);
      return;
    }
    backPrev = backNow;

    delay(20);
  }

  // Country entry
  uiShowPromptId(TXT_ENTER_COUNTRY);
  char lineBuf[21];
  snprintf(lineBuf, sizeof(lineBuf), "%-20s", city);
  int citylen = strlen(city);
  if (citylen < 18) { lineBuf[citylen] = ' '; lineBuf[citylen+1] = country[0]; lineBuf[citylen+2] = country[1]; }
  if (currentLanguage == LANG_EN) enPrintFixed(0,1,lineBuf); else grPrintFixed(0,1,lineBuf);
  if (currentLanguage == LANG_EN) enPrintFixed(0,3,"SEL:SAVE BK:CANCEL"); else grPrintFixed(0,3,"SEL:ΑΠΟΘ BK:ΑΚΥΡ");

  int cpos = 0;
  unsigned long lastBlink2 = millis(); bool blinkOn2 = true;
  bool upPrev2=false, downPrev2=false, selPrev2=false, backPrev2=false;
  unsigned long selHoldStart2 = 0;

  while (true) {
    unsigned long now = millis();
    if (now - lastBlink2 >= CURSOR_BLINK_MS) { lastBlink2 = now; blinkOn2 = !blinkOn2; }

    bool upNow = (digitalRead(BTN_UP) == LOW);
    bool downNow = (digitalRead(BTN_DOWN) == LOW);
    bool selNow = (digitalRead(BTN_SELECT) == LOW);
    bool backNow = (digitalRead(BTN_BACK) == LOW);

    // UP/DOWN for country chars
    if (upNow && !upPrev2) { char *p = strchr(charset, country[cpos]); if(!p) country[cpos]='A'; else { int idx=(p - charset); idx=(idx+1)%(int)strlen(charset); country[cpos]=charset[idx]; } }
    if (downNow && !downPrev2) { char *p = strchr(charset, country[cpos]); if(!p) country[cpos]='Z'; else { int idx=(p - charset); idx=(idx-1+(int)strlen(charset))%(int)strlen(charset); country[cpos]=charset[idx]; } }

    // SELECT handling: support long-press to save immediately
    if (selNow && !selPrev2) {
      // start hold timer
      selHoldStart2 = now;
      // short-press behavior: advance caret / save when at end
      if (cpos == 0) { cpos = 1; }
      else {
        bool countryIsSpaces = (country[0] == ' ' && country[1] == ' ');
        bool ok;
        if (countryIsSpaces) ok = weather_geocodeLocation(city, nullptr);
        else ok = weather_geocodeLocation(city, country);

        uiClear();
        if (ok) {
          if (currentLanguage==LANG_EN) enPrintFixed(0,0,getTextEN(TXT_GEOCODE_SAVED));
          else grPrintFixed(0,0,getTextGR(TXT_GEOCODE_SAVED));
          // attempt immediate weather fetch to populate forecast and verify
          if (WiFi.status() == WL_CONNECTED) {
            if (weather_fetch()) {
              if (currentLanguage==LANG_EN) enPrintFixed(0,1,"WEATHER FETCH OK");
              else grPrintFixed(0,1,"WEATHER FETCH OK");
            } else {
              if (currentLanguage==LANG_EN) enPrintFixed(0,1,"WEATHER FETCH FAIL");
              else grPrintFixed(0,1,"WEATHER FAIL");
            }
          }
        } else {
          if (currentLanguage==LANG_EN) enPrintFixed(0,0,getTextEN(TXT_GEOCODE_FAILED));
          else grPrintFixed(0,0,getTextGR(TXT_GEOCODE_FAILED));
        }
        delay(900);
        menuDraw();
        return;
      }
    } else if (selNow && selPrev2) {
      // held - check for save threshold
      if (selHoldStart2 && (now - selHoldStart2 >= SELECT_SAVE_HOLD_MS)) {
        // Save immediately (long-press) and exit
        bool countryIsSpaces = (country[0] == ' ' && country[1] == ' ');
        bool ok;
        if (countryIsSpaces) ok = weather_geocodeLocation(city, nullptr);
        else ok = weather_geocodeLocation(city, country);

        uiClear();
        if (ok) {
          if (currentLanguage==LANG_EN) enPrintFixed(0,0,getTextEN(TXT_GEOCODE_SAVED));
          else grPrintFixed(0,0,getTextGR(TXT_GEOCODE_SAVED));
          // attempt immediate weather fetch to populate forecast and verify
          if (WiFi.status() == WL_CONNECTED) {
            if (weather_fetch()) {
              if (currentLanguage==LANG_EN) enPrintFixed(0,1,"WEATHER FETCH OK");
              else grPrintFixed(0,1,"WEATHER FETCH OK");
            } else {
              if (currentLanguage==LANG_EN) enPrintFixed(0,1,"WEATHER FETCH FAIL");
              else grPrintFixed(0,1,"WEATHER FAIL");
            }
          }
        } else {
          if (currentLanguage==LANG_EN) enPrintFixed(0,0,getTextEN(TXT_GEOCODE_FAILED));
          else grPrintFixed(0,0,getTextGR(TXT_GEOCODE_FAILED));
        }
        delay(900);
        menuDraw();
        return;
      }
    }
    selPrev2 = selNow;

    if (backNow && !backPrev2) { menuDraw(); return; }
    backPrev2 = backNow;

    // redraw country line and caret
    snprintf(lineBuf, sizeof(lineBuf), "%-20s", city);
    if (citylen < 18) { lineBuf[citylen] = ' '; lineBuf[citylen+1] = country[0]; if (cpos>0) lineBuf[citylen+2]=country[1]; }
    if (currentLanguage==LANG_EN) enPrintFixed(0,1,lineBuf); else grPrintFixed(0,1,lineBuf);

    char caretLine[21]; for (int i=0;i<20;i++) caretLine[i]=' '; caretLine[20]=0;
    int caretPos = citylen + 1 + cpos;
    if (caretPos>=0 && caretPos<20) caretLine[caretPos] = (blinkOn2 ? '^' : ' ');
    if (currentLanguage==LANG_EN) enPrintFixed(0,2,caretLine); else grPrintFixed(0,2,caretLine);

    delay(60);
  }
}