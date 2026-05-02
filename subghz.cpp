#include "hardware.h"
#include "subghz.h"

// CC1101 SPI interface
static SPIClass *cc1101SPI = nullptr;
static bool spiInitialized = false;

bool subghzScanning = false;
float subghzFreq = 433.92;
int subghzCapturedCount = 0;
int lastSubghzSlot = -1;
bool cc1101Present = false;
SubGHzSignal subghzSignals[MAX_SUBGHZ_SIGNALS];

// ── SPI Communication ──

static void cc1101Select() {
  digitalWrite(CC1101_CS, LOW);
  delayMicroseconds(10);
}

static void cc1101Deselect() {
  digitalWrite(CC1101_CS, HIGH);
  delayMicroseconds(10);
}

uint8_t cc1101ReadReg(uint8_t addr) {
  cc1101Select();
  cc1101SPI->transfer(addr | 0x80);  // R/W bit
  uint8_t val = cc1101SPI->transfer(0x00);
  cc1101Deselect();
  return val;
}

void cc1101WriteReg(uint8_t addr, uint8_t val) {
  cc1101Select();
  cc1101SPI->transfer(addr & 0x7F);  // Write: bit7=0
  cc1101SPI->transfer(val);
  cc1101Deselect();
}

uint8_t cc1101Strobe(uint8_t cmd) {
  cc1101Select();
  uint8_t status = cc1101SPI->transfer(cmd);
  cc1101Deselect();
  return status;
}

void cc1101Reset() {
  cc1101Deselect();
  delayMicroseconds(50);
  cc1101Select();
  delayMicroseconds(50);
  cc1101Deselect();
  delayMicroseconds(50);
  cc1101Strobe(CC1101_SRES);
  delay(100);
}

// ── Configuration ──

void cc1101Config433() {
  cc1101Reset();
  // 433.92 MHz OOK configuration
  cc1101WriteReg(CC1101_IOCFG0,   0x06);  // GDO0 assertion on RX
  cc1101WriteReg(CC1101_FIFOTHR,  0x07);  // FIFO threshold
  cc1101WriteReg(CC1101_PKTCTRL0, 0x32);  // Variable length, whitening
  cc1101WriteReg(CC1101_FSCTRL1,  0x06);  // Freq synthesizer
  // 433.92 MHz = 0x10, 0x7B, 0x3B (from CC1101 calculator)
  cc1101WriteReg(CC1101_FREQ2,    0x10);
  cc1101WriteReg(CC1101_FREQ1,    0x7B);
  cc1101WriteReg(CC1101_FREQ0,    0x3B);
  cc1101WriteReg(CC1101_MDMCFG4,  0xF6);  // Modem config
  cc1101WriteReg(CC1101_MDMCFG3,  0x1D);
  cc1101WriteReg(CC1101_MDMCFG2,  0x30);  // OOK modulation
  cc1101WriteReg(CC1101_DEVIATN,  0x14);
  cc1101WriteReg(CC1101_MCSM0,    0x18);
  cc1101WriteReg(CC1101_FOCCFG,   0x16);
  cc1101WriteReg(CC1101_BSCFG,    0x1C);
  cc1101WriteReg(CC1101_AGCCTRL2, 0xC7);
  cc1101WriteReg(CC1101_AGCCTRL1, 0x00);
  cc1101WriteReg(CC1101_AGCCTRL0, 0xB0);
  cc1101WriteReg(CC1101_FREND1,   0x00);
  cc1101WriteReg(CC1101_FREND0,   0x11);
  cc1101WriteReg(CC1101_FSCAL3,   0xE9);
  cc1101WriteReg(CC1101_FSCAL2,   0x2A);
  cc1101WriteReg(CC1101_FSCAL1,   0x00);
  cc1101WriteReg(CC1101_FSCAL0,   0x1F);
  cc1101WriteReg(CC1101_TEST2,    0x81);
  cc1101WriteReg(CC1101_TEST1,    0x35);
  cc1101WriteReg(CC1101_TEST0,    0x09);
}

void cc1101Config868() {
  cc1101Reset();
  // 868.00 MHz FSK configuration
  cc1101WriteReg(CC1101_IOCFG0,   0x06);
  cc1101WriteReg(CC1101_FIFOTHR,  0x07);
  cc1101WriteReg(CC1101_PKTCTRL0, 0x32);
  cc1101WriteReg(CC1101_FSCTRL1,  0x06);
  // 868.00 MHz = 0x21, 0x62, 0x76
  cc1101WriteReg(CC1101_FREQ2,    0x21);
  cc1101WriteReg(CC1101_FREQ1,    0x62);
  cc1101WriteReg(CC1101_FREQ0,    0x76);
  cc1101WriteReg(CC1101_MDMCFG4,  0xF6);
  cc1101WriteReg(CC1101_MDMCFG3,  0x1D);
  cc1101WriteReg(CC1101_MDMCFG2,  0x30);  // OOK
  cc1101WriteReg(CC1101_DEVIATN,  0x14);
  cc1101WriteReg(CC1101_MCSM0,    0x18);
  cc1101WriteReg(CC1101_FOCCFG,   0x16);
  cc1101WriteReg(CC1101_BSCFG,    0x1C);
  cc1101WriteReg(CC1101_AGCCTRL2, 0xC7);
  cc1101WriteReg(CC1101_AGCCTRL1, 0x00);
  cc1101WriteReg(CC1101_AGCCTRL0, 0xB0);
  cc1101WriteReg(CC1101_FREND1,   0x00);
  cc1101WriteReg(CC1101_FREND0,   0x11);
  cc1101WriteReg(CC1101_FSCAL3,   0xE9);
  cc1101WriteReg(CC1101_FSCAL2,   0x2A);
  cc1101WriteReg(CC1101_FSCAL1,   0x00);
  cc1101WriteReg(CC1101_FSCAL0,   0x1F);
  cc1101WriteReg(CC1101_TEST2,    0x81);
  cc1101WriteReg(CC1101_TEST1,    0x35);
  cc1101WriteReg(CC1101_TEST0,    0x09);
}

// ── Public API ──

void initSubGHz() {
  pinMode(CC1101_CS, OUTPUT);
  digitalWrite(CC1101_CS, HIGH);

  cc1101SPI = new SPIClass(VSPI);
  cc1101SPI->begin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CS);

  // Test if CC1101 is present
  cc1101Reset();
  uint8_t partnum = cc1101ReadReg(0x30);  // PARTNUM
  uint8_t version = cc1101ReadReg(0x31);  // VERSION

  cc1101Present = (partnum != 0x00 && partnum != 0xFF);
  if (cc1101Present) {
    Serial.printf("CC1101 detected: partnum=0x%02X version=0x%02X\n", partnum, version);
  } else {
    Serial.println("CC1101 not detected — Sub-GHz disabled");
  }

  subghzScanning = false;
  subghzCapturedCount = 0;
  for (int i = 0; i < MAX_SUBGHZ_SIGNALS; i++) subghzSignals[i].valid = false;
}

void setSubGHzFreq(float freqMHz) {
  subghzFreq = freqMHz;
  if (!cc1101Present) return;

  if (fabsf(freqMHz - 433.92) < 0.1) {
    cc1101Config433();
  } else if (fabsf(freqMHz - 868.0) < 0.1) {
    cc1101Config868();
  }
}

void startSubGHzScan(float freqMHz) {
  if (!cc1101Present) {
    Serial.println("CC1101 not present, cannot scan");
    return;
  }
  setSubGHzFreq(freqMHz);
  cc1101Strobe(CC1101_SRX);  // Enter RX mode
  subghzScanning = true;
  Serial.printf("Sub-GHz scanning at %.2f MHz\n", freqMHz);
}

void stopSubGHzScan() {
  if (cc1101Present) {
    cc1101Strobe(CC1101_SIDLE);  // Exit RX
  }
  subghzScanning = false;
}

// ── GDO0 Interrupt for pulse capture ──

static volatile bool gdo0Rising = false;
static volatile unsigned long gdo0LastTime = 0;
static volatile uint16_t gdo0PulseCount = 0;
static volatile bool gdo0Overflow = false;
static uint32_t gdo0Pulses[256];  // pulse widths in microseconds
static const uint16_t GDO0_MAX_PULSES = 256;

void IRAM_ATTR onGDO0Change() {
  unsigned long now = micros();
  bool high = digitalRead(CC1101_GDO0);
  if (gdo0PulseCount > 0 || high) {
    if (gdo0PulseCount < GDO0_MAX_PULSES) {
      gdo0Pulses[gdo0PulseCount] = now - gdo0LastTime;
      gdo0PulseCount++;
    } else {
      gdo0Overflow = true;
    }
  }
  gdo0LastTime = now;
}

void captureSubGHz() {
  if (!cc1101Present || !subghzScanning) return;

  // Attach interrupt and wait for signal
  gdo0PulseCount = 0;
  gdo0Overflow = false;
  gdo0LastTime = micros();
  attachInterrupt(digitalPinToInterrupt(CC1101_GDO0), onGDO0Change, CHANGE);

  // Wait for capture (up to 500ms)
  unsigned long start = millis();
  while (gdo0PulseCount < 4 && millis() - start < 500) {
    delay(1);
  }

  detachInterrupt(digitalPinToInterrupt(CC1101_GDO0));

  if (gdo0PulseCount < 4) return;  // not enough data

  // Store captured signal
  if (subghzCapturedCount < MAX_SUBGHZ_SIGNALS) {
    SubGHzSignal &sig = subghzSignals[subghzCapturedCount];
    sig.frequency = subghzFreq;
    sig.timestamp = millis();
    sig.pulseCount = gdo0PulseCount;
    if (gdo0PulseCount > 64) sig.pulseCount = 64;  // cap at struct size
    for (uint16_t i = 0; i < sig.pulseCount && i < 64; i++) {
      sig.pulseWidths[i] = gdo0Pulses[i];
    }
    strncpy(sig.modulation, "OOK", sizeof(sig.modulation) - 1);
    sig.isRollingCode = false;
    sig.valid = true;
    lastSubghzSlot = subghzCapturedCount;
    subghzCapturedCount++;
  }
}

void replaySubGHz() {
  replaySubGHzSlot(lastSubghzSlot);
}

void replaySubGHzSlot(int slot) {
  if (slot < 0 || slot >= subghzCapturedCount || !subghzSignals[slot].valid) return;
  if (!cc1101Present) return;

  SubGHzSignal &sig = subghzSignals[slot];

  // Set frequency and enter TX
  setSubGHzFreq(sig.frequency);
  cc1101Strobe(CC1101_STX);
  delay(1);  // Let TX settle

  // Transmit captured pulse pattern via GDO0
  // Configure GDO0 as output for manual TX modulation
  pinMode(CC1101_GDO0, OUTPUT);
  for (uint16_t i = 0; i < sig.pulseCount; i++) {
    digitalWrite(CC1101_GDO0, (i % 2 == 0) ? HIGH : LOW);
    delayMicroseconds(sig.pulseWidths[i] > 0 ? sig.pulseWidths[i] : 100);
  }
  digitalWrite(CC1101_GDO0, LOW);

  cc1101Strobe(CC1101_SIDLE);

  // Reconfigure GDO0 as input for RX
  pinMode(CC1101_GDO0, INPUT);
}

String getSubGHzStatusJSON() {
  return "{\"scanning\":" + String(subghzScanning ? "true" : "false") +
         ",\"freq\":" + String(subghzFreq, 2) +
         ",\"captured\":" + String(subghzCapturedCount) +
         ",\"hardware\":" + String(cc1101Present ? "\"CC1101\"" : "\"none\"") + "}";
}

String getSubGHzCapturedJSON() {
  String json = "[";
  for (int i = 0; i < subghzCapturedCount; i++) {
    if (i) json += ",";
    SubGHzSignal &s = subghzSignals[i];
    json += "{\"slot\":" + String(i) +
           ",\"freq\":" + String(s.frequency, 2) +
           ",\"modulation\":\"" + String(s.modulation) + "\"" +
           ",\"pulses\":" + String(s.pulseCount) +
           ",\"rolling\":" + String(s.isRollingCode ? "true" : "false") + "}";
  }
  json += "]";
  return json;
}

String getSubGHzFrequenciesJSON() {
  return "[{\"freq\":433.92,\"label\":\"433 MHz (doorbell, sensors)\"}" +
         String(",{\"freq\":868.00,\"label\":\"868 MHz (EU sensors)\"}" +
                ",{\"freq\":315.00,\"label\":\"315 MHz (US/JP)\"}]");
}

void subghzLoop() {
  if (!subghzScanning || !cc1101Present) return;

  // Auto-capture: when scanning and GDO0 goes high, a signal may be present
  // Use manual capture via web API for more control
}