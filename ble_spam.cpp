#include "hardware.h"
#include "ble_spam.h"

#ifdef ENABLE_BLE_SPAM
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#endif

bool bleSpamActive = false;
int bleCurrentProfile = 0;

BLEDeviceProfile bleProfiles[] = {
  {"AirPods Pro", "airpods", {0x07,0xFF,0x4C,0x00,0x07,0x0A,0x02,0x00,0x01}, 9},
  {"AirPods Max", "airpods", {0x07,0xFF,0x4C,0x00,0x07,0x0A,0x0A,0x00,0x01}, 9},
  {"Samsung Buds", "samsung", {0x0A,0xFF,0x75,0x00,0x01,0x00,0x02,0x00,0x01,0x03,0x01}, 11},
  {"Pixel Buds", "pixel",   {0x02,0x01,0x06,0x03,0x03,0xAA,0xFE}, 7},
  {"Apple TV", "appletv",    {0x05,0xFF,0x4C,0x00,0x04,0x0C}, 6},
  {"Tile Tracker", "tile",  {0x02,0x01,0x06,0x04,0xFF,0xFE,0x00}, 7}
};
const int NUM_BLE_PROFILES = 6;

#ifdef ENABLE_BLE_SPAM

void initBLESpam() {
  bleSpamActive = false;
  bleCurrentProfile = 0;
}

void startBLESpam() {
  bleSpamActive = true;
}

void stopBLESpam() {
  bleSpamActive = false;
  BLEDevice::deinit(false);
}

void updateBLESpam() {
  static unsigned long lastBleCycle = 0;
  if (!bleSpamActive) return;
  if (millis() - lastBleCycle < 2000) return;
  lastBleCycle = millis();

  BLEDevice::deinit(false);
  BLEDevice::init("FlipperSpam");

  BLEServer *pServer = BLEDevice::createServer();
  BLEAdvertising *pAdv = pServer->getAdvertising();

  BLEDeviceProfile &prof = bleProfiles[bleCurrentProfile];
  pAdv->setAdvertisementData(BLEAdvertisementData(prof.advData, prof.advDataLen));

  pAdv->setMinInterval(0x20);
  pAdv->setMaxInterval(0x20);
  pAdv->start();

  bleCurrentProfile = (bleCurrentProfile + 1) % NUM_BLE_PROFILES;
}

#else

void initBLESpam() {}
void startBLESpam() {}
void stopBLESpam() {}
void updateBLESpam() {}

#endif