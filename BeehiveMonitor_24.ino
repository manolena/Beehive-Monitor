#define TINY_GSM_MODEM_A7670
#define TINY_GSM_RX_BUFFER 1024

#include "config.h"
#include "ui.h"
#include "menu_manager.h"
#include "text_strings.h"
#include "modem_manager.h"
#include "time_manager.h"
// debug_inject_key removed (no OpenWeather key support)
// #include "debug_inject_key.h"   // declares debug_injectKeyNow(...)
#include "weather_manager.h"    // declares weather_init(), weather_fetch(), etc.
#include "key_server.h"
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

// -----------------------------------------------------------------------------
// Single global language selection (definition)
// Provide one definition here so the linker finds currentLanguage used by ui.cpp
Language currentLanguage = LANG_EN;

// -----------------------------------------------------------------------------
// Define the single global LCD instance (ui.cpp declares extern LiquidCrystal_I2C lcd;)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ============================================
// PLACEHOLDER SENSOR VALUES FOR TESTING
// ============================================
float test_weight = 12.4;

float test_temp_int = 32.5;
float test_hum_int = 58.0;

float test_temp_ext = 28.8;
float test_hum_ext = 64.0;
float test_pressure = 1012.3;

float test_acc_x = 0.02;
float test_acc_y = -0.01;
float test_acc_z = 0.98;

float test_batt_voltage = 4.12;
int test_batt_percent = 87;

int test_rssi = -72;

// -----------------------------------------------------------------------------
// Helper: connect to WiFi using credentials stored in Preferences (optional).
// Preferences keys: namespace "wifi_cfg", keys "ssid" and "pass".
// This is non-blocking with timeout and only used for convenience during testing.
static const char* PREF_WIFI_NS = "wifi_cfg";
static const char* PREF_WIFI_SSID = "ssid";
static const char* PREF_WIFI_PASS = "pass";

void wifi_connectFromPrefs(unsigned long timeoutMs = 8000) {
  Preferences p;
  p.begin(PREF_WIFI_NS, true);
  String ssid = p.getString(PREF_WIFI_SSID, "");
  String pass = p.getString(PREF_WIFI_PASS, "");
  p.end();

  if (ssid.length() == 0) {
    Serial.println("[WiFi] No SSID stored in prefs");
    return;
  }

  Serial.print("[WiFi] Connecting to SSID: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Connect timed out");
  }

  // Debug: dump coords + request URL and attempt a weather fetch
  weather_debug_dumpAndFetch();
}

// ============================================
// Setup / Loop
// ============================================
void setup() {

  // Serial early so we can see logs during init
  Serial.begin(115200);
  delay(50);

  uiInit();

  // configure button pins early so menu/UI can read them
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  // ----------------------------
  // SD INIT (GLOBAL)
  // ----------------------------
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  bool sd_ok = SD.begin(SD_CS);

  // initialize other modules
  weather_init();

  // Try to connect to WiFi using stored prefs (optional convenience)
  wifi_connectFromPrefs(8000);

  // If WiFi is already connected start the key server (provisioning server)
  if (WiFi.status() == WL_CONNECTED) keyServer_init();

  // Removed debug_injectKeyNow — no API key injection any more

  Serial.println(sd_ok ? "SD init OK" : "SD init FAIL");

  showSplashScreen();

  menuInit();
  modemManager_init();   // modem manager stubs / init
  timeManager_init();

  // If WiFi connected we already started keyServer; keyServer_loop() will keep it alive.
}

// Main loop: UI/menu + time manager + key-server loop
void loop() {

  menuUpdate();
  timeManager_update();  // <-- REQUIRED for status screen timing

  // key server (provisioning) - auto-starts when WiFi connects (safe to call always)
  keyServer_loop();

  delay(10);
}

void showSplashScreen() {
  lcd.clear();

  if (currentLanguage == LANG_EN) {
    lcd.setCursor(0, 0);
    lcd.print("====================");
    lcd.setCursor(0, 1);
    lcd.print("  BEEHIVE MONITOR   ");
    lcd.setCursor(0, 2);
    lcd.print("        v23         ");
    lcd.setCursor(0, 3);
    lcd.print("====================");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("====================");
    lcdPrintGreek(" ΠΑΡΑΚΟΛΟΥΘΗΣΗ     ", 0, 1);
    lcdPrintGreek("  ΚΥΨΕΛΗΣ v23       ", 0, 2);
    lcd.setCursor(0, 3);
    lcd.print("====================");
  }
  delay(1000);
}