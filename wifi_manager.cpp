#include "hardware.h"
#include "wifi_manager.h"
#include <Preferences.h>

static Preferences prefs;
static String savedSSID = "";
static String savedPass = "";
static unsigned long lastWiFiCheck = 0;

void initWiFiManager() {
  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", "");
  savedPass = prefs.getString("pass", "");
  prefs.end();
}

bool connectToSavedWiFi() {
  if (savedSSID.length() == 0) return false;
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(200);
  }
  return WiFi.status() == WL_CONNECTED;
}

void saveWiFiCredentials(const char* ssid, const char* password) {
  savedSSID = ssid;
  savedPass = password;
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", password);
  prefs.end();
}

void forgetWiFi() {
  savedSSID = "";
  savedPass = "";
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  WiFi.disconnect(true);
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void checkWiFiReconnect() {
  if (millis() - lastWiFiCheck < WIFI_CHECK_INTERVAL) return;
  lastWiFiCheck = millis();
  if (savedSSID.length() > 0 && WiFi.status() != WL_CONNECTED) {
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  }
}

String getWiFiStatusJSON() {
  return "{\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") +
         ",\"ssid\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "") + "\"" +
         ",\"ip\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0") + "\"}";
}

String getWiFiScanJSON() {
  int n = WiFi.scanNetworks(false, false, false, 300);
  String json = "[";
  for (int i = 0; i < n && i < MAX_SCAN_RESULTS; i++) {
    if (i) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"enc\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]";
  WiFi.scanDelete();
  return json;
}