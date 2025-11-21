// key_server.cpp
// - Simple HTTP provisioning server that accepts city + country for geocoding.
// - When started it prints the IP to Serial and shows it briefly on the LCD (row 3).
#include "key_server.h"
#include "weather_manager.h"
#include "ui.h"
#include "menu_manager.h"
#include <WiFi.h>

static WiFiServer *s_server = nullptr;
static unsigned long s_lastActivity = 0;
static const unsigned long IDLE_TIMEOUT_MS = 5 * 60 * 1000UL; // stop server after idle
static bool s_running = false;

static String urlDecode(const String &src) {
  String ret;
  char tmp[3] = {0,0,0};
  for (size_t i=0; i < src.length(); ++i) {
    char c = src[i];
    if (c == '+') ret += ' ';
    else if (c == '%' && i + 2 < src.length()) {
      tmp[0] = src[i+1];
      tmp[1] = src[i+2];
      int v = (int)strtol(tmp, nullptr, 16);
      ret += (char)v;
      i += 2;
    } else ret += c;
  }
  return ret;
}

static void sendHttpResponse(WiFiClient &client, const String &body) {
  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: text/html; charset=UTF-8\r\n");
  client.print("Connection: close\r\n");
  client.printf("Content-Length: %u\r\n\r\n", (unsigned)body.length());
  client.print(body);
}

static String makeFormPage(const String &status) {
  String page;
  page.reserve(1024);
  page += "<!doctype html><html><head><meta charset='utf-8'><title>Beehive: Provision</title></head><body>";
  page += "<h3>Beehive Provisioning</h3>";
  if (status.length()) {
    page += "<p><b>Status:</b> ";
    page += status;
    page += "</p>";
  }

  page += "<p><i>Enter City and (optional) 2-letter Country code. Location will be used with Open‑Meteo (no API key required).</i></p>";

  page += "<form method='POST' action='/set'>";
  page += "City: <input name='city' style='width:200px' placeholder='e.g. Elefsina'> Country (2-letter ISO): <input name='country' style='width:60px' placeholder='GR'><br><br>";
  page += "<input type='submit' value='Save'>";
  page += "</form>";
  page += "<p>Or use GET: /set?city=Athens&country=GR</p>";
  page += "</body></html>";
  return page;
}

void keyServer_init() {
  if (s_server) return;
  s_server = new WiFiServer(80);
  s_server->begin();
  s_running = true;
  s_lastActivity = millis();
  Serial.println("[KeyServer] started on port 80");
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    Serial.print("[KeyServer] IP: ");
    Serial.println(ip);

    // Show IP on LCD row 3 briefly so user can open browser
    char buf[21];
    snprintf(buf, sizeof(buf), "IP: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    uiPrint(0, 3, buf);
    delay(3500);
    // restore menu screen if menu exists; attempt to redraw menu
    menuDraw();
  }
}

// Stop server
void keyServer_stop() {
  if (!s_server) return;
  s_server->stop();
  delete s_server;
  s_server = nullptr;
  s_running = false;
  Serial.println("[KeyServer] stopped");
}

// Call this periodically from loop(); it will auto-start server when WiFi connects.
void keyServer_loop() {
  // Stop server on idle
  if (s_running && (millis() - s_lastActivity > IDLE_TIMEOUT_MS)) {
    Serial.println("[KeyServer] idle timeout, stopping");
    keyServer_stop();
  }

  // Auto-start when WiFi connects
#if AUTOSTART_KEYSERVER
  if (!s_server && WiFi.status() == WL_CONNECTED) {
    keyServer_init();
  }
#endif

  if (!s_server) return;

  WiFiClient client = s_server->available();
  if (!client) return;

  s_lastActivity = millis();
  Serial.println("[KeyServer] client connected");

  // Read request headers (simple)
  String req;
  unsigned long start = millis();
  while (client.connected() && (millis() - start < 2000)) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      req += line + '\n';
      if (line.length() == 1 && line[0] == '\r') break;
    } else delay(1);
  }

  int idx = req.indexOf('\n');
  String reqLine = idx >= 0 ? req.substring(0, idx) : String();
  reqLine.trim();
  Serial.print("[KeyServer] Request: ");
  Serial.println(reqLine);

  // Parse method and path
  String method, path;
  {
    int sp1 = reqLine.indexOf(' ');
    int sp2 = (sp1 >= 0) ? reqLine.indexOf(' ', sp1 + 1) : -1;
    if (sp1 >= 0 && sp2 > sp1) {
      method = reqLine.substring(0, sp1);
      path = reqLine.substring(sp1 + 1, sp2);
    }
  }

  String body;
  if (method == "POST") {
    int clIdx = req.indexOf("\nContent-Length:");
    int contentLength = 0;
    if (clIdx < 0) clIdx = req.indexOf("\ncontent-length:");
    if (clIdx >= 0) {
      int lineEnd = req.indexOf('\n', clIdx + 1);
      String clLine = req.substring(clIdx + 1, lineEnd);
      clLine.replace("Content-Length:", "");
      clLine.replace("content-length:", "");
      clLine.trim();
      contentLength = clLine.toInt();
    }
    unsigned long tstart = millis();
    while (client.connected() && (int)body.length() < contentLength && (millis() - tstart < 2000)) {
      if (client.available()) {
        char c = client.read();
        body += c;
      } else delay(1);
    }
  }

  // Extract query string
  String query;
  int qIdx = path.indexOf('?');
  if (qIdx >= 0) {
    query = path.substring(qIdx + 1);
    path = path.substring(0, qIdx);
  } else if (method == "POST") {
    query = body;
  }

  // Root page
  if (path == "/" || path.length() == 0) {
    String page = makeFormPage("");
    sendHttpResponse(client, page);
    client.stop();
    return;
  }

  if (path == "/set") {
    // parse query k=v&...
    String valCity = "", valCountry = "";
    int pos = 0;
    while (pos < (int)query.length()) {
      int amp = query.indexOf('&', pos);
      String part;
      if (amp >= 0) { part = query.substring(pos, amp); pos = amp + 1; }
      else { part = query.substring(pos); pos = query.length(); }
      int eq = part.indexOf('=');
      if (eq < 0) continue;
      String name = part.substring(0, eq);
      String val = urlDecode(part.substring(eq+1));
      name.trim();
      val.trim();
      if (name == "city") valCity = val;
      else if (name == "country") valCountry = val;
    }

    String statusMsg;

    if (valCity.length() > 0) {
      bool geoOk = false;
      if (valCountry.length() > 0) geoOk = weather_geocodeLocation(valCity.c_str(), valCountry.c_str());
      else geoOk = weather_geocodeLocation(valCity.c_str(), nullptr);
      if (geoOk) { statusMsg += "Geocode OK. "; Serial.println("[KeyServer] Geocode OK"); }
      else { statusMsg += "Geocode failed. "; Serial.print("[KeyServer] Geocode failed: "); Serial.println(weather_getLastError()); }
    } else {
      statusMsg += "No city provided. ";
    }

    // Verify fetch if WiFi connected
    if (WiFi.status() == WL_CONNECTED) {
      if (weather_fetch()) {
        statusMsg += "Weather fetch OK.";
      } else {
        statusMsg += "Weather fetch failed.";
      }
    } else {
      statusMsg += "WiFi not connected for verify.";
    }

    String page = makeFormPage(statusMsg);
    sendHttpResponse(client, page);
    client.stop();
    s_lastActivity = millis();
    return;
  }

  // other paths -> 404
  {
    String page = "<html><body><h3>404</h3></body></html>";
    sendHttpResponse(client, page);
    client.stop();
    return;
  }
}