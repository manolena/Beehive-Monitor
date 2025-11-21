#include "sms_handler.h"
#include "modem_manager.h"
#include "weather_manager.h"
#include "text_strings.h"
#include <TinyGsmClient.h>
#include <Arduino.h>

// This SMS handler uses AT commands sent through the TinyGsm modem instance.
// It attempts to:
// - set text mode (AT+CMGF=1)
// - list unread messages (AT+CMGL="REC UNREAD")
// - parse messages for commands (GEO:city,country)
// - on success call weather_geocodeLocation()
// - delete processed messages (AT+CMGD=index)
// - attempt to send a basic SMS reply confirming the action (AT+CMGS)

static unsigned long s_lastCheck = 0;
static const unsigned long SMS_CHECK_INTERVAL = 30UL * 1000UL; // check every 30s

void sms_init() {
  // Ensure modem is initialized externally (modemManager_init)
  TinyGsm &modem = modem_get();
  Serial.println("[SMS] Setting text mode (AT+CMGF=1)...");
  modem.sendAT("+CMGF=1");
  modem.waitResponse(2000);
  s_lastCheck = millis();
}

// Helper: send AT and read stream for a short time, return aggregated response
static String modemReadResponse(unsigned long timeout = 3000) {
  TinyGsm &modem = modem_get();
  Stream &s = modem.stream;
  unsigned long t0 = millis();
  String resp;
  while (millis() - t0 < timeout) {
    while (s.available()) {
      char c = s.read();
      resp += c;
    }
    delay(10);
  }
  return resp;
}

// Helper: send AT and get response quickly
static String atSendAndRead(const char *cmd, unsigned long timeout = 3000) {
  TinyGsm &modem = modem_get();
  modem.sendAT(cmd);
  return modemReadResponse(timeout);
}

// Attempt to send a text SMS (best-effort). number must be in international format.
static bool sms_send(const String &number, const String &message) {
  TinyGsm &modem = modem_get();
  // Set text mode first (already done), then send AT+CMGS="num"
  String at = String("+CMGS=\"") + number + "\"";
  modem.sendAT(at.c_str());
  // wait for '>' prompt
  unsigned long t0 = millis();
  bool prompt = false;
  while (millis() - t0 < 3000) {
    while (modem.stream.available()) {
      char c = modem.stream.read();
      if (c == '>') { prompt = true; break; }
    }
    if (prompt) break;
    delay(10);
  }
  if (!prompt) {
    Serial.println("[SMS] No prompt for CMGS");
    return false;
  }
  // send message then Ctrl+Z
  modem.stream.print(message);
  modem.stream.write(26); // Ctrl+Z
  // read response
  String r = modemReadResponse(5000);
  Serial.print("[SMS] send response: "); Serial.println(r);
  if (r.indexOf("OK") >= 0) return true;
  return false;
}

// Parse and handle messages returned by AT+CMGL
static void processSmsListResponse(const String &resp) {
  int idx = 0;
  while (true) {
    int pos = resp.indexOf("\n+CMGL:", idx);
    if (pos < 0) {
      if (idx == 0) pos = resp.indexOf("+CMGL:");
      if (pos < 0) break;
    }
    int eol = resp.indexOf('\n', pos+1);
    if (eol < 0) break;
    String header = resp.substring(pos, eol);
    int cidx = -1;
    {
      int p1 = header.indexOf(':');
      int p2 = header.indexOf(',');
      if (p1 >= 0 && p2 > p1) {
        String sidx = header.substring(p1+1, p2);
        sidx.trim();
        cidx = sidx.toInt();
      }
    }
    int bodyStart = eol + 1;
    int nextHeader = resp.indexOf("\n+CMGL:", bodyStart);
    if (nextHeader < 0) nextHeader = resp.length();
    String body = resp.substring(bodyStart, nextHeader);
    body.trim();
    if (body.length() == 0) {
      idx = nextHeader;
      continue;
    }
    Serial.print("[SMS] Msg idx="); Serial.print(cidx); Serial.print(" body='"); Serial.print(body); Serial.println("'");
    String u = body;
    u.toUpperCase();
    bool handled = false;

    // Support GEO:city,country messages only; ignore API key commands
    if (u.startsWith("GEO:")) {
      int colon = body.indexOf(':');
      String payload = (colon>=0) ? body.substring(colon+1) : "";
      payload.trim();
      int comma = payload.indexOf(',');
      String city = payload;
      String country = "";
      if (comma >= 0) {
        city = payload.substring(0, comma);
        country = payload.substring(comma+1);
      }
      city.trim();
      country.trim();
      if (city.length() > 0) {
        if (weather_geocodeLocation(city.c_str(), country.length() ? country.c_str() : nullptr)) {
          Serial.println("[SMS] Geocode stored from SMS");
          int q1 = header.indexOf('"', header.indexOf(',')+1);
          int q2 = -1;
          if (q1 >= 0) q2 = header.indexOf('"', q1+1);
          String from = "";
          if (q1 >= 0 && q2 > q1) from = header.substring(q1+1, q2);
          if (from.length()) sms_send(from, "OK: Geocode stored");
          handled = true;
        } else {
          Serial.print("[SMS] Geocode failed: ");
          Serial.println(weather_getLastError());
        }
      }
    } else {
      Serial.println("[SMS] Unknown or unsupported command in SMS");
    }

    if (cidx >= 0) {
      char cmd[32];
      snprintf(cmd, sizeof(cmd), "+CMGD=%d", cidx);
      TinyGsm &modem = modem_get();
      modem.sendAT(cmd);
      modem.waitResponse(2000);
    }

    idx = nextHeader;
  } // while
}

void sms_loop() {
  if (millis() - s_lastCheck < SMS_CHECK_INTERVAL) return;
  s_lastCheck = millis();

  TinyGsm &modem = modem_get();
  Serial.println("[SMS] Checking unread messages...");
  modem.sendAT("+CMGF=1");
  modem.waitResponse(1000);

  modem.sendAT("+CMGL=\"REC UNREAD\"");
  String resp = modemReadResponse(3000);
  Serial.print("[SMS] CMGL response: "); Serial.println(resp);

  if (resp.indexOf("+CMGL:") >= 0) {
    processSmsListResponse(resp);
  } else {
    Serial.println("[SMS] No unread messages");
  }
}