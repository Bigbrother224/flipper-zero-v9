#include "hardware.h"
#include "ir_control.h"
#include <IRutils.h>

IRrecv irReceiver(IR_RX_PIN, IR_RAW_BUF_SIZE);
IRsend irSender(IR_TX_PIN);

// Move TV power codes from header to avoid duplicate symbols
const IRPresetCode tvPowerCodes[] = {
  {"Samsung",    decode_type_t::SAMSUNG,    0xE0E040BF, 32},
  {"Samsung",    decode_type_t::SAMSUNG,    0xE0E019E6, 32},
  {"LG",         decode_type_t::LG,        0x20DF10EF, 32},
  {"LG",         decode_type_t::LG,        0x20DFA35C, 32},
  {"Sony",       decode_type_t::SONY,     0xA90,      12},
  {"Sony",       decode_type_t::SONY,     0x490,      12},
  {"Panasonic",  decode_type_t::PANASONIC, 0x40040100707, 48},
  {"Toshiba",    decode_type_t::NEC,      0xF72CD827, 32},
  {"Philips",    decode_type_t::RC5,      0x0C,       12},
  {"Sharp",      decode_type_t::SHARP,    0x410A05FA, 32},
};
const int TV_CODE_COUNT = 10;

// uint64 to String helper (not provided by all Arduino cores)
static String uint64ToHexString(uint64_t val) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%llX", (unsigned long long)val);
  return "0x" + String(buf);
}

bool irCapturing = false;
int irCapturedCount = 0;
int lastCapturedSlot = -1;
IRCapturedSignal irSlots[MAX_IR_SLOTS];
static unsigned long irCaptureStart = 0;
static decode_results irDecodeResults;

void initIR() {
  irSender.begin();
  irReceiver.enableIRIn();
  irCapturing = false;
  irCapturedCount = 0;
  lastCapturedSlot = -1;
  for (int i = 0; i < MAX_IR_SLOTS; i++) irSlots[i].valid = false;
}

void startIRCapture() {
  irReceiver.enableIRIn();
  irCapturing = true;
  irCaptureStart = millis();
}

void stopIRCapture() {
  irCapturing = false;
}

void replayLastIR() {
  if (lastCapturedSlot < 0) return;
  replayIRSlot(lastCapturedSlot);
}

void replayIRSlot(int slot) {
  if (slot < 0 || slot >= MAX_IR_SLOTS || !irSlots[slot].valid) return;

  IRCapturedSignal &sig = irSlots[slot];

  if (sig.protocol != decode_type_t::UNKNOWN) {
    // Known protocol — send as protocol + code
    irSender.send(sig.protocol, sig.code, sig.bits);
  } else if (sig.rawLen > 0) {
    // Unknown protocol — send raw timings
    irSender.sendRaw(sig.rawBuf, sig.rawLen, 38);
  }
}

void sendIRCode(decode_type_t protocol, uint64_t code, uint16_t bits) {
  irSender.send(protocol, code, bits);
}

void bruteForceTV(const char* brand) {
  String b = String(brand);
  b.toLowerCase();

  for (int i = 0; i < TV_CODE_COUNT; i++) {
    String codeBrand = String(tvPowerCodes[i].brand);
    codeBrand.toLowerCase();

    if (b == "all" || b == codeBrand) {
      irSender.send(tvPowerCodes[i].protocol, tvPowerCodes[i].code, tvPowerCodes[i].bits);
      delay(200);  // gap between codes
    }
  }
}

void clearIRSlots() {
  irCapturedCount = 0;
  lastCapturedSlot = -1;
  for (int i = 0; i < MAX_IR_SLOTS; i++) irSlots[i].valid = false;
}

String getIRStatusJSON() {
  return "{\"capturing\":" + String(irCapturing ? "true" : "false") +
         ",\"captured\":" + String(irCapturedCount) +
         ",\"lastSlot\":" + String(lastCapturedSlot) + "}";
}

String getIRCapturedJSON() {
  String json = "[";
  for (int i = 0; i < irCapturedCount; i++) {
    if (i) json += ",";
    IRCapturedSignal &s = irSlots[i];
    json += "{\"slot\":" + String(i) +
           ",\"protocol\":\"" + typeToString(s.protocol) + "\"" +
           ",\"code\":\"" + uint64ToHexString(s.code) + "\"" +
           ",\"bits\":" + String(s.bits) +
           ",\"valid\":" + String(s.valid ? "true" : "false") + "}";
  }
  json += "]";
  return json;
}

String getIRPresetsJSON() {
  String json = "[";
  for (int i = 0; i < TV_CODE_COUNT; i++) {
    if (i) json += ",";
    json += "{\"brand\":\"" + String(tvPowerCodes[i].brand) + "\"" +
           ",\"protocol\":\"" + typeToString(tvPowerCodes[i].protocol) + "\"" +
           ",\"code\":\"" + uint64ToHexString(tvPowerCodes[i].code) + "\"" +
           ",\"bits\":" + String(tvPowerCodes[i].bits) + "}";
  }
  json += "]";
  return json;
}

void irLoop() {
  if (!irCapturing) return;

  // Timeout
  if (millis() - irCaptureStart > IR_CAPTURE_TIMEOUT) {
    irCapturing = false;
    return;
  }

  if (irReceiver.decode(&irDecodeResults)) {
    if (irDecodeResults.value != 0 && irCapturedCount < MAX_IR_SLOTS) {
      int slot = irCapturedCount;
      irSlots[slot].protocol = irDecodeResults.decode_type;
      irSlots[slot].code = irDecodeResults.value;
      irSlots[slot].bits = irDecodeResults.bits;
      irSlots[slot].valid = true;

      // Store raw data for unknown protocols
      if (irDecodeResults.decode_type == decode_type_t::UNKNOWN &&
          irDecodeResults.rawlen < 200) {
        irSlots[slot].rawLen = irDecodeResults.rawlen;
        for (uint16_t i = 0; i < irDecodeResults.rawlen && i < 200; i++) {
          irSlots[slot].rawBuf[i] = irDecodeResults.rawbuf[i];
        }
      } else {
        irSlots[slot].rawLen = 0;
      }

      irCapturedCount++;
      lastCapturedSlot = slot;
    }
    irReceiver.resume();
  }
}