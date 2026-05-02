#pragma once
#include <Arduino.h>
#include <SPI.h>

// ── Sub-GHz Module — CC1101 via SPI ──
// Full implementation requires CC1101 hardware module
// Structure is ready; SPI init + register config is placeholder

#define CC1101_FREQ_433 433.92
#define CC1101_FREQ_868 868.00
#define CC1101_FREQ_315 315.00
#define MAX_SUBGHZ_SIGNALS 10

// CC1101 Register addresses
#define CC1101_IOCFG0    0x00
#define CC1101_FIFOTHR   0x03
#define CC1101_PKTLEN   0x06
#define CC1101_PKTCTRL0 0x08
#define CC1101_FSCTRL1   0x0B
#define CC1101_FREQ2     0x0D
#define CC1101_FREQ1     0x0E
#define CC1101_FREQ0     0x0F
#define CC1101_MDMCFG4   0x10
#define CC1101_MDMCFG3  0x11
#define CC1101_MDMCFG2  0x12
#define CC1101_DEVIATN  0x15
#define CC1101_MCSM0    0x18
#define CC1101_FOCCFG   0x19
#define CC1101_BSCFG    0x1A
#define CC1101_AGCCTRL2 0x1B
#define CC1101_AGCCTRL1 0x1C
#define CC1101_AGCCTRL0 0x1D
#define CC1101_FREND1   0x21
#define CC1101_FREND0   0x22
#define CC1101_FSCAL3   0x23
#define CC1101_FSCAL2   0x24
#define CC1101_FSCAL1   0x25
#define CC1101_FSCAL0   0x26
#define CC1101_TEST2    0x2C
#define CC1101_TEST1    0x2D
#define CC1101_TEST0    0x2E
#define CC1101_PKTCTRL1 0x07

// CC1101 command strobes (from TI CC1101 datasheet)
#define CC1101_SRES     0x30  // Reset chip
#define CC1101_SFSTXON  0x31  // Enable and calibrate frequency synthesizer
#define CC1101_SXOFF    0x32  // Turn off crystal oscillator
#define CC1101_SCAL     0x33  // Calibrate frequency synthesizer and turn it off
#define CC1101_SRX      0x34  // Enable RX
#define CC1101_STX      0x35  // Enable TX (when MCSM0.FS_AUTOCAL=01, goes to IDLE first)
#define CC1101_SIDLE    0x36  // Exit RX/TX, turn off frequency synthesizer
#define CC1101_SNOP     0x3D  // No operation — returns status byte

// CC1101 status
#define CC1101_STATUS_TX      0x20
#define CC1101_STATUS_RX      0x10
#define CC1101_STATUS_IDLE    0x00

struct SubGHzSignal {
  float frequency;
  uint32_t timestamp;
  uint16_t pulseCount;
  uint32_t pulseWidths[64];  // microseconds
  char modulation[16];        // "OOK" or "FSK"
  bool isRollingCode;
  bool valid;
};

void initSubGHz();
void startSubGHzScan(float freqMHz);
void stopSubGHzScan();
void captureSubGHz();
void replaySubGHz();
void replaySubGHzSlot(int slot);
void setSubGHzFreq(float freqMHz);
String getSubGHzStatusJSON();
String getSubGHzCapturedJSON();
String getSubGHzFrequenciesJSON();

// Low-level CC1101 SPI
uint8_t cc1101ReadReg(uint8_t addr);
void cc1101WriteReg(uint8_t addr, uint8_t val);
uint8_t cc1101Strobe(uint8_t cmd);
void cc1101Reset();
void cc1101Config433();
void cc1101Config868();

void subghzLoop();

extern bool subghzScanning;
extern float subghzFreq;
extern int subghzCapturedCount;
extern SubGHzSignal subghzSignals[];
extern int lastSubghzSlot;
extern bool cc1101Present;