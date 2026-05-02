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

// Pre-defined power codes
static const IRPresetCode tvPowerCodes[] = {
  {"Samsung",    decode_type_t::SAMSUNG,    0xE0E040BF, 32},
  {"Samsung",    decode_type_t::SAMSUNG,    0xE0E019E6, 32},  // Samsung power alt
  {"LG",         decode_type_t::LG,        0x20DF10EF, 32},
  {"LG",         decode_type_t::LG,        0x20DFA35C, 32},  // LG power alt
  {"Sony",       decode_type_t::SONY,     0xA90,      12},
  {"Sony",       decode_type_t::SONY,     0x490,      12},   // Sony power alt
  {"Panasonic",  decode_type_t::PANASONIC, 0x40040100707, 48},
  {"Toshiba",    decode_type_t::NEC,      0xF72CD827, 32},
  {"Philips",    decode_type_t::RC5,      0x0C,       12},
  {"Sharp",      decode_type_t::SHARP,    0x410A05FA, 32},
};
static const int TV_CODE_COUNT = 10;