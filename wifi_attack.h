#pragma once
#include <Arduino.h>
#include <WiFi.h>

// ── WiFi Attack Module ──

extern "C" {
  esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);
}

struct ScanResult {
  char ssid[33];
  uint8_t bssid[6];
  int rssi;
  int channel;
  bool encrypted;
};

extern ScanResult scannedNets[];
extern uint8_t scannedBSSID[][6];
extern int scanCount;
extern bool attackActive;
extern String targetBSSID;
extern int targetChannel;
extern String targetSSID;

void initWiFiAttack();
void performScan();
String getScanJSON();
void startDeauthAttack();
void startTargetedDeauth();
void stopAttack();
String getAttackStatus();
void wifiAttackLoop();  // call from loop() — non-blocking