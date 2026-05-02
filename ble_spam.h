#pragma once
#include <Arduino.h>

// ── BLE Spam Module ──

extern bool bleSpamActive;
extern int bleCurrentProfile;

void initBLESpam();
void startBLESpam();
void stopBLESpam();
void updateBLESpam();  // call from loop()