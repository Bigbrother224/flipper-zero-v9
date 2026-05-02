#pragma once
#include <Arduino.h>

// ── WiFi Manager Module ──

void initWiFiManager();
bool connectToSavedWiFi();
void saveWiFiCredentials(const char* ssid, const char* password);
void forgetWiFi();
bool isWiFiConnected();
String getWiFiStatusJSON();
String getWiFiScanJSON();