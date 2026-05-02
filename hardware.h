#pragma once
// ── Reaper v9 — Hardware Pin Definitions ──

// LCD 16x2 I2C (PCF8574)
#define LCD_SDA       21
#define LCD_SCL       22
#define LCD_ADDR      0x27  // common PCF8574 address (or 0x3F)
#define LCD_COLS      16
#define LCD_ROWS      2

// Joystick (analog)
#define JOY_VRX       34   // X axis (ADC1)
#define JOY_VRY       35   // Y axis (ADC1)
#define JOY_SW        27   // Push button
#define JOY_THRESHOLD 1200 // ADC threshold (center ~2048)

// IR
#define IR_TX_PIN     25
#define IR_RX_PIN     26

// CC1101 Sub-GHz (SPI)
#define CC1101_MOSI   23
#define CC1101_MISO   19
#define CC1101_SCK    18
#define CC1101_CS     5
#define CC1101_GDO0   15
#define CC1101_GDO2   4

// WiFi AP defaults
#define AP_SSID       "Reaper"
#define AP_PASSWORD   "r34p3r_t00l"

// System
#define WDT_TIMEOUT           30
#define WIFI_CHECK_INTERVAL   30000
#define MAX_CAPTURED_CREDS    20
#define MAX_PROXY_CLIENTS     4
#define PROXY_BUF_SIZE        4096
#define PROXY_LOG_SIZE        30
#define MAX_SCAN_RESULTS      30
