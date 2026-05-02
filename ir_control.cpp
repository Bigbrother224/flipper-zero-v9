#include "hardware.h"
#include "ir_control.h"

IRrecv irReceiver(IR_RX_PIN, IR_RAW_BUF_SIZE);
IRsend irSender(IR_TX_PIN);

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
           ",\"code\":\"0x" + uint64ToString(s.code, 16) + "\"" +
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
           ",\"code\":\"0x" + uint64ToString(tvPowerCodes[i].code, 16) + "\"" +
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