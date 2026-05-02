#include "hardware.h"
#include "wifi_manager.h"
#include <Preferences.h>
#include <ArduinoJson.h>

static Preferences prefs;
static String savedSSID = "";
static String savedPass = "";
static unsigned long lastWiFiCheck = 0;
static unsigned long wifiConnectStart = 0;
static bool wifiConnecting = false;

// ── Simple XOR obfuscation for stored credentials ──
static const uint8_t XOR_KEY[] = {0x7A, 0x3F, 0xE1, 0x94, 0x0B, 0x5C, 0xD8, 0x26};

static String xorObfuscate(const String& input) {
  String out = input;
  for (unsigned int i = 0; i < out.length(); i++) {
    out[i] = input[i] ^ XOR_KEY[i % sizeof(XOR_KEY)];
  }
  return out;
}

void initWiFiManager() {
  prefs.begin("wifi", true);
  String encSSID = prefs.getString("ssid", "");
  String encPass = prefs.getString("pass", "");
  prefs.end();

  savedSSID = xorObfuscate(encSSID);
  savedPass = xorObfuscate(encPass);
}

bool connectToSavedWiFi() {
  if (savedSSID.length() == 0) return false;
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  wifiConnectStart = millis();
  wifiConnecting = true;
  return true;
}

void saveWiFiCredentials(const char* ssid, const char* password) {
  savedSSID = ssid;
  savedPass = password;

  prefs.begin("wifi", false);
  prefs.putString("ssid", xorObfuscate(ssid));
  prefs.putString("pass", xorObfuscate(password));
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

// ── Non-blocking reconnect ──
void checkWiFiReconnect() {
  // Check pending connection
  if (wifiConnecting) {
    if (WiFi.status() == WL_CONNECTED || millis() - wifiConnectStart > 15000) {
      wifiConnecting = false;
    }
    return;
  }

  if (millis() - lastWiFiCheck < WIFI_CHECK_INTERVAL) return;
  lastWiFiCheck = millis();

  if (savedSSID.length() > 0 && WiFi.status() != WL_CONNECTED) {
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    wifiConnectStart = millis();
    wifiConnecting = true;
  }
}

String getWiFiStatusJSON() {
  JsonDocument doc;
  doc["connected"] = WiFi.status() == WL_CONNECTED;
  doc["ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0";
  doc["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  String out;
  serializeJson(doc, out);
  return out;
}

String getWiFiScanJSON() {
  int n = WiFi.scanNetworks(false, false, false, 300);
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < n && i < MAX_SCAN_RESULTS; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["ssid"] = WiFi.SSID(i);
    obj["rssi"] = WiFi.RSSI(i);
    obj["enc"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  String out;
  serializeJson(doc, out);
  return out;
}
