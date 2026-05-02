#pragma once
#include <Arduino.h>

// ── BLE Spam Module ──

struct BLEDeviceProfile {
  const char* name;
  const char* type;
  uint8_t advData[28];
  uint8_t advDataLen;
};

extern BLEDeviceProfile bleProfiles[];
extern const int NUM_BLE_PROFILES;
extern bool bleSpamActive;
extern int bleCurrentProfile;

void initBLESpam();
void startBLESpam();
void stopBLESpam();
void updateBLESpam();  // call from loop()