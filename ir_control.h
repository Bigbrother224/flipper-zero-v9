#pragma once
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

// ── IR Module — Full Implementation with IRremoteESP8266 ──

#define IR_CAPTURE_TIMEOUT 15000  // 15 seconds max capture
#define IR_RAW_BUF_SIZE    1024
#define MAX_IR_SLOTS       10      // stored captured signals

struct IRCapturedSignal {
  decode_type_t protocol;
  uint64_t code;
  uint16_t bits;
  uint16_t rawLen;
  uint16_t rawBuf[200];  // raw timing data for replay
  bool valid;
};

// Pre-defined TV power codes
struct IRPresetCode {
  const char* brand;
  decode_type_t protocol;
  uint64_t code;
  uint16_t bits;
};

void initIR();
void startIRCapture();
void stopIRCapture();
void replayLastIR();
void replayIRSlot(int slot);
void sendIRCode(decode_type_t protocol, uint64_t code, uint16_t bits);
void bruteForceTV(const char* brand);
void clearIRSlots();
String getIRStatusJSON();
String getIRCapturedJSON();
String getIRPresetsJSON();

// Call from loop()
void irLoop();

extern bool irCapturing;
extern int irCapturedCount;
extern IRCapturedSignal irSlots[];
extern int lastCapturedSlot;

// TV codes defined in ir_control.cpp
extern const IRPresetCode tvPowerCodes[];
extern const int TV_CODE_COUNT;