# Flipper Zero v9

ESP32 security research multi-tool. WiFi deauth, BLE spam, captive portal, TCP proxy, IR capture/replay, Sub-GHz scanning — all from a 16x2 LCD + joystick interface.

Built for the field. No screen needed — the web dashboard runs on the AP at `192.168.4.1`.

---

## Features

### WiFi
- **Scanner** — passive network scan with BSSID, RSSI, channel, encryption
- **Deauth** — broadcast or targeted 802.11 deauth frames with MAC randomization
- **Channel hopping** — cycles through channels 1-13 during broadcast deauth

### BLE Spam
- Cycles through 6 device profiles: AirPods Pro, AirPods Max, Samsung Buds, Pixel Buds, Apple TV, Tile
- Rapid advertisement cycling every 2 seconds

### Captive Portal
- 5 template styles: WiFi login, hotel, ISP update, airport, cafe
- Credential capture with IP logging
- DNS hijack for forced redirect
- Clear/manage creds via API

### TCP Proxy
- Multi-client TCP relay (up to 4 simultaneous)
- Traffic logging with method, host, bytes in/out
- Configurable port (default 8080)

### Sub-GHz (CC1101)
- Scan 433 MHz and 868 MHz bands
- OOK/FSK modulation support
- Signal capture and replay framework
- Auto-detects CC1101 presence on boot

### IR
- Capture any IR remote protocol (NEC, Samsung, Sony, RC5, etc.)
- Raw timing capture for unknown protocols
- Replay from captured slots
- Built-in TV power codes: Samsung, LG, Sony, Panasonic, Toshiba, Philips, Sharp

### LCD Menu
- 16x2 LCD I2C display (PCF8574) with joystick navigation
- Nested menu system with toggle states
- Status overlay with auto-timeout
- Web dashboard at `http://192.168.4.1/` for full control

---

## Hardware

| Component | Notes |
|---|---|
| ESP32 WROOM | Main MCU |
| 16x2 LCD I2C | PCF8574 backpack, addr 0x27 or 0x3F |
| Analog joystick | VRX, VRY, SW (push) |
| IR LED | TX on GPIO 25 |
| IR Receiver | RX on GPIO 26 |
| CC1101 module | Sub-GHz radio via SPI |

## Wiring

```
LCD 16x2 I2C
  SDA ─── GPIO 21
  SCL ─── GPIO 22

Analog Joystick
  VRX ─── GPIO 34 (ADC1)
  VRY ─── GPIO 35 (ADC1)
  SW  ─── GPIO 27 (INPUT_PULLUP)

IR
  TX  ─── GPIO 25
  RX  ─── GPIO 26

CC1101 Sub-GHz (SPI)
  MOSI ── GPIO 23
  MISO ── GPIO 19
  SCK  ── GPIO 18
  CS   ── GPIO 5
  GDO0 ── GPIO 15
  GDO2 ── GPIO 4
```

---

## Quick Start

1. Install [Arduino IDE](https://www.arduino.cc/en/software) or PlatformIO
2. Install ESP32 board support (Board Manager → `esp32`)
3. Install libraries:
   - `LiquidCrystal_I2C`
   - `IRremoteESP8266`
   - `ArduinoJson`
   - `ESP32 BLE`
4. Open `FlipperZero_Style_v9.ino`
5. Select board: **ESP32 Dev Module**
6. Upload
7. Connect to WiFi AP: `FlipperPro_v9` / `smarttool`
8. Open `http://192.168.4.1/` in a browser

---

## API

All endpoints return JSON.

| Endpoint | Method | Description |
|---|---|---|
| `/status` | GET | System status (attack, BLE, portal, proxy, heap) |
| `/scan` | GET | WiFi network scan |
| `/attack` | GET | Start deauth (opt: `?target=BSSID&ch=N`) |
| `/stop` | GET | Stop deauth attack |
| `/ble/start` | GET | Start BLE spam |
| `/ble/stop` | GET | Stop BLE spam |
| `/portal/start` | GET | Start captive portal (opt: `?t=0-4`) |
| `/portal/stop` | GET | Stop portal |
| `/portal/creds` | GET | Captured credentials |
| `/portal/clear` | GET | Clear captured creds |
| `/proxy/start` | GET | Start proxy (opt: `?port=8080`) |
| `/proxy/stop` | GET | Stop proxy |
| `/proxy/log` | GET | Proxy traffic log |
| `/ir/capture` | GET | Start IR capture |
| `/ir/stop` | GET | Stop capture, return results |
| `/ir/replay` | GET | Replay last captured signal |
| `/subghz/scan` | GET | Start Sub-GHz scan (`?freq=433.92`) |
| `/subghz/stop` | GET | Stop Sub-GHz scan |

---

## Tech Stack

- **C++** / Arduino framework
- **ESP32** (WROOM, dual-core, WiFi+BLE)
- **CC1101** Sub-GHz radio (SPI)
- **IRremoteESP8266** for IR protocols
- **ArduinoJson** for API responses
- **LiquidCrystal_I2C** for display

---

## Disclaimer

This tool is built for **authorized security research and educational purposes only**.

Deauth frames, captive portals, BLE spoofing, and signal replay can be illegal when used without explicit permission from the network/equipment owner. You are solely responsible for ensuring your use complies with all applicable laws.

**Do not use this tool on networks or devices you do not own or have authorization to test.** The authors accept no liability for misuse.

---

## License

MIT