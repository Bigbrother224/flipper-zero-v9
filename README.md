<div align="center">

<img src="https://img.shields.io/badge/ESP32-WROOM-00979D?style=for-the-badge&logo=espressif&logoColor=white" alt="ESP32">
<img src="https://img.shields.io/badge/Arduino-3.3.7-00979D?style=for-the-badge&logo=arduino&logoColor=white" alt="Arduino">
<img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" alt="License">
<img src="https://img.shields.io/badge/Security-Research-red?style=for-the-badge" alt="Security">

<br><br>

```
╔══════════════════════════════════════════════════════════════╗
║   ███████╗██╗     ██╗██████╗ ██████╗ ███████╗██████╗          ║
║   ██╔════╝██║     ██║██╔══██╗██╔══██╗██╔════╝██╔══██╗         ║
║   █████╗  ██║     ██║██████╔╝██████╔╝█████╗  ██████╔╝         ║
║   ██╔══╝  ██║     ██║██╔═══╝ ██╔═══╝ ██╔══╝  ██╔══██╗         ║
║   ██║     ███████╗██║██║     ██║     ███████╗██║  ██║         ║
║   ╚═╝     ╚══════╝╚═╝╚═╝     ╚═╝     ╚══════╝╚═╝  ╚═╝         ║
║                                                              ║
║              ESP32 Security Research Multi-Tool              ║
║                      v9 — "The Reaper"                       ║
╚══════════════════════════════════════════════════════════════╝
```

**WiFi Deauth · BLE Spam · Captive Portal · TCP Proxy · IR Capture/Replay · Sub-GHz Scanner**

Built for **red teams, hardware hackers, and security researchers** who need a portable, field-ready multi-tool. No external screen required — the 16×2 LCD + joystick handle standalone ops, and the **web dashboard** gives you full control from any phone.

</div>

---

## Table of Contents

- [What This Tool Does](#-what-this-tool-does)
- [Features Deep-Dive](#-features-deep-dive)
- [Hardware Requirements](#-hardware-requirements)
- [Wiring](#-wiring)
- [Quick Start](#-quick-start)
- [Web Dashboard](#-web-dashboard)
- [API Reference](#-api-reference)
- [Project Structure](#-project-structure)
- [Security & Legal](#-security--legal)
- [License](#-license)

---

## 🎯 What This Tool Does

This is an **ESP32-based offensive security multi-tool** inspired by the Flipper Zero form factor. It packs 6 attack surfaces into a single Arduino sketch with a modular C++ architecture:

<div align="center">

| Module | Attack Surface | Real Hardware? |
|---|---|---|
| 📡 **WiFi** | 802.11 deauth frames via `esp_wifi_80211_tx` | ✅ Native ESP32 |
| 📶 **BLE** | Advertisement spoofing (6 device profiles) | ✅ Native ESP32 |
| 🎣 **Portal** | DNS hijack + credential harvesting (5 templates) | ✅ Native ESP32 |
| 🔄 **Proxy** | TCP/HTTP forward proxy + traffic logging | ✅ Native ESP32 |
| 📡 **Sub-GHz** | OOK/FSK scan & capture (433/868 MHz) | 🔧 CC1101 module |
| 🔴 **IR** | Multi-protocol capture & replay (NEC/Sony/Samsung/RC5) | 🔧 IR LED + TSOP |

</div>

All modules are controlled from a unified **16×2 LCD joystick menu** or the **REST API at `192.168.4.1`**.

---

## 🔥 Features Deep-Dive

### 📡 WiFi Attack Suite

```cpp
// Real 802.11 management frames — not just TCP packets
esp_wifi_80211_tx(WIFI_IF_STA, packet, size, true);
```

- **Passive Scanner** — BSSID, RSSI, channel, encryption type for every AP in range
- **Broadcast Deauth** — kicks all clients on all visible APs, channel-hopping across 1-13
- **Targeted Deauth** — single AP, single channel, MAC address randomization
- **Channel Hopper** — cycles 1-13 during broadcast mode for maximum coverage

> **ESP32 Caveat**: `esp_wifi_80211_tx` on `WIFI_IF_STA` may silently fail if not connected to a real AP. Connect to a network via **WiFi Manager** first for reliable deauth.

### 📶 BLE Advertisement Spam

Cycles through **6 device profiles** every 2 seconds. Designed to flood BLE scanners and cause confusion at close range:

| Profile | Icon | Behavior |
|---|---|---|
| AirPods Pro | 🎧 | Proximity pairing popup triggers |
| AirPods Max | 🎧 | Over-ear headphone spoof |
| Samsung Buds | 🎧 | Galaxy ecosystem trigger |
| Pixel Buds | 🎧 | Google Fast Pair spam |
| Apple TV | 📺 | Apple TV remote pairing prompt |
| Tile | 🔍 | Bluetooth tracker spoof |

### 🎣 Captive Portal — 5 Templates

DNS hijack forces every HTTP request to the portal. Credentials are logged with **IP, username, password, and timestamp**.

| # | Template | Gradient | Target Language |
|---|---|---|---|
| 0 | WiFi Login | Blue `#2563eb` | FR |
| 1 | Hotel WiFi | Green `#059669` | FR |
| 2 | ISP Update | Amber `#d97706` | FR |
| 3 | Airport WiFi | Purple `#7c3aed` | FR |
| 4 | Café WiFi | Orange `#ea580c` | FR |

Each portal is a self-contained HTML/CSS page — no external dependencies, fully offline.

### 🔄 TCP Proxy

Multi-client TCP relay with traffic inspection:

- **Up to 4 simultaneous connections** on FreeRTOS core 1
- **Request logging**: HTTP method, host header, bytes in/out
- **HTTPS CONNECT tunneling** — transparent pass-through on port 443
- **Configurable port** (default `8080`)

### 📡 Sub-GHz (CC1101 via SPI)

- **Dual-band scan**: 433.92 MHz and 868.00 MHz
- **OOK + FSK modulation** support
- **Signal capture & replay** framework
- **Auto-detection**: checks for CC1101 on boot, gracefully disables if absent

### 🔴 IR Control

- **Universal capture** — NEC, Samsung, Sony, RC5, RC6, Panasonic, JVC, Mitsubishi
- **Raw timing capture** for unknown/obscure protocols
- **Replay from captured slots**
- **Built-in TV power codes**: Samsung, LG, Sony, Panasonic, Toshiba, Philips, Sharp

### 🖥️ LCD Menu System

- **16×2 LCD I2C** (PCF8574 backpack, `0x27` or `0x3F`)
- **Analog joystick** (VRX/VRY/SW) navigation
- **Nested menus** with toggle states for each module
- **Status overlay** with auto-timeout after actions
- Falls back to **web dashboard at `http://192.168.4.1/`** for full control

---

## 🔩 Hardware Requirements

| Component | Spec | Required? |
|---|---|---|
| **ESP32 WROOM** | Dual-core, WiFi + BLE | ✅ Core |
| **16×2 LCD I2C** | PCF8574 backpack, addr `0x27` or `0x3F` | ✅ Core |
| **Analog Joystick** | VRX, VRY, SW (push) | ✅ Core |
| **IR LED** | TX on GPIO 25 | 🔧 IR only |
| **IR Receiver** | TSOP38238 or similar, GPIO 26 | 🔧 IR capture |
| **CC1101 Module** | Sub-GHz radio via SPI | 🔧 Sub-GHz only |
| **Breadboard + Jumper Wires** | Male-to-male + male-to-female | ✅ Build |

---

## ⚡ Wiring

```
┌─────────────────────────────────────────────────────────┐
│                      ESP32 WROOM                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │  21─▸ SDA ──────────── LCD 16x2 I2C              │   │
│  │  22─▸ SCL ──────────── (PCF8574, 0x27/0x3F)      │   │
│  │                                                    │   │
│  │  34─▸ VRX ──────────── Analog Joystick            │   │
│  │  35─▸ VRY                                         │   │
│  │  27─▸ SW  (INPUT_PULLUP)                          │   │
│  │                                                    │   │
│  │  25─▸ IR LED TX                                    │   │
│  │  26─▸ IR Receiver RX (TSOP38238)                   │   │
│  │                                                    │   │
│  │  23─▸ CC1101 MOSI                                  │   │
│  │  19─▸ CC1101 MISO                                  │   │
│  │  18─▸ CC1101 SCK                                   │   │
│  │   5─▸ CC1101 CS                                    │   │
│  │  15─▸ CC1101 GDO0                                  │   │
│  │   4─▸ CC1101 GDO2                                  │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

<details>
<summary><b>Raw Pin Reference (click to expand)</b></summary>

### LCD 16×2 I2C
```
SDA ─── GPIO 21
SCL ─── GPIO 22
VCC ─── 5V
GND ─── GND
```

### Analog Joystick
```
VRX ─── GPIO 34 (ADC1)
VRY ─── GPIO 35 (ADC1)
SW  ─── GPIO 27 (INPUT_PULLUP)
VCC ─── 3.3V
GND ─── GND
```

### IR
```
TX ─── GPIO 25
RX ─── GPIO 26
```

### CC1101 Sub-GHz (SPI)
```
MOSI ── GPIO 23
MISO ── GPIO 19
SCK  ── GPIO 18
CS   ── GPIO 5
GDO0 ── GPIO 15
GDO2 ── GPIO 4
VCC  ── 3.3V
GND  ── GND
```

</details>

---

## 🚀 Quick Start

### 1. Install Toolchain

- [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/)
- ESP32 board support: **Board Manager → `esp32` by Espressif Systems (v3.0+)**
- Partition scheme: **`Huge APP (3MB No OTA/1MB SPIFFS)`**

### 2. Install Libraries

| Library | Version | Purpose |
|---|---|---|
| `LiquidCrystal_I2C` | latest | LCD driver |
| `IRremoteESP8266` | latest | IR capture/replay |
| `ArduinoJson` | ^7.4 | API JSON serialization |
| `ESP32 BLE Arduino` | built-in | BLE advertisement spam |

### 3. Flash & Connect

```bash
# 1. Open FlipperZero_Style_v9.ino in Arduino IDE
# 2. Board: ESP32 Dev Module
# 3. Partition Scheme: Huge APP
# 4. Hit Upload

# After boot:
SSID:     FlipperPro_v9
Password: smarttool

# Open the dashboard:
http://192.168.4.1/
# or via mDNS:
http://flipper.local/
```

---

## 🖥️ Web Dashboard

The AP at `192.168.4.1` serves a responsive dashboard with **live status cards**:

<div align="center">

| Card | Live Data |
|---|---|
| **Deauth** | Status, RSSI, target BSSID, heap memory |
| **Sub-GHz** | Scan status, current frequency, captured count |
| **IR** | Capture/replay status, last protocol detected |
| **Captive Portal** | Active template, credential count, last capture IP |
| **Proxy** | Port, running status, client count |

</div>

The dashboard auto-refreshes every 3 seconds. No npm, no dependencies — the full UI is embedded in the firmware as a single `PROGMEM` string.

> 📸 **Screenshots**: Drop dashboard captures in `./assets/` and link them here.  
> Use `![Dashboard](assets/dashboard.png)` and `![LCD Menu](assets/lcd-menu.jpg)`.

---

## 📡 API Reference

> **Base URL**: `http://192.168.4.1/`  
> **All endpoints return JSON**. No authentication required (local AP only).

### System

<details>
<summary><code>GET /status</code> — System health & active modules</summary>

```bash
curl -s http://192.168.4.1/status | jq .
```
```json
{
  "deauth": false,
  "ble": true,
  "portal": false,
  "proxy": false,
  "heap": 214536,
  "uptime": 843
}
```
</details>

### WiFi Attacks

| Endpoint | Params | Description |
|---|---|---|
| `GET /scan` | — | Passive AP scan, returns JSON array |
| `GET /attack` | `?target=AA:BB:CC:DD:EE:FF&ch=6` | Targeted deauth (omit `target` for broadcast) |
| `GET /stop` | — | Stop all WiFi attacks |

```bash
# Scan nearby APs
curl -s http://192.168.4.1/scan | jq '.[] | {ssid, rssi, channel, encryption}'

# Deauth a specific AP on channel 6
curl -s http://192.168.4.1/attack?target=AA:BB:CC:DD:EE:FF\&ch=6

# Broadcast deauth (all APs, all channels)
curl -s http://192.168.4.1/attack

# Stop
curl -s http://192.168.4.1/stop
```

### BLE Spam

| Endpoint | Description |
|---|---|
| `GET /ble/start` | Start BLE spam cycling |
| `GET /ble/stop` | Stop BLE spam |

### Captive Portal

| Endpoint | Params | Description |
|---|---|---|
| `GET /portal/start` | `?t=0-4` | Start portal (0=WiFi, 1=Hotel, 2=ISP, 3=Airport, 4=Café) |
| `GET /portal/stop` | — | Stop portal + DNS hijack |
| `GET /portal/creds` | — | Dump captured credentials as JSON |
| `GET /portal/clear` | — | Wipe captured credentials |

```bash
# Start hotel WiFi portal
curl -s http://192.168.4.1/portal/start?t=1

# Get captured creds
curl -s http://192.168.4.1/portal/creds | jq .
```

### Proxy

| Endpoint | Params | Description |
|---|---|---|
| `GET /proxy/start` | `?port=8080` | Start TCP proxy on given port |
| `GET /proxy/stop` | — | Stop proxy, dump traffic log |
| `GET /proxy/log` | — | Get traffic log with method/host/bytes |

### IR

| Endpoint | Description |
|---|---|
| `GET /ir/capture` | Start IR capture session |
| `GET /ir/stop` | End capture, returns protocol + raw timing |
| `GET /ir/replay` | Replay last captured IR signal |

```bash
# Capture and replay workflow
curl -s http://192.168.4.1/ir/capture
# ... point remote at IR receiver ...
curl -s http://192.168.4.1/ir/stop | jq .
curl -s http://192.168.4.1/ir/replay
```

### Sub-GHz

| Endpoint | Params | Description |
|---|---|---|
| `GET /subghz/scan` | `?freq=433.92` | Start Sub-GHz scan at given frequency |
| `GET /subghz/stop` | — | Stop scan |

```bash
# Scan 433 MHz band
curl -s http://192.168.4.1/subghz/scan?freq=433.92

# Scan 868 MHz band
curl -s http://192.168.4.1/subghz/scan?freq=868.00
```

---

## 🗂️ Project Structure

```
FlipperZero_Style_v9/
├── FlipperZero_Style_v9.ino   # Main sketch — web server, API routes, menu init
├── hardware.h                  # Pin definitions, shared constants
│
├── wifi_attack.cpp/.h          # 802.11 deauth (broadcast + targeted)
├── wifi_manager.cpp/.h         # AP mode, mDNS, STA auto-connect
├── ble_spam.cpp/.h             # BLE advertisement cycling (6+ profiles)
├── captive_portal.cpp/.h       # DNS hijack + 5 portal templates + cred storage
├── proxy.cpp/.h                # TCP/HTTP proxy + HTTPS CONNECT tunnel
├── subghz.cpp/.h               # CC1101 SPI driver + OOK/FSK scan
├── ir_control.cpp/.h           # IR capture (IRremoteESP8266) + replay
├── oled_menu.cpp/.h            # 16×2 LCD I2C + joystick menu engine
│
└── README.md                   # ← you are here
```

**1744 lines total.** Every module is self-contained — add new attack surfaces by dropping a `.cpp/.h` pair and registering it in the menu.

---

## ⚠️ Security & Legal

<div align="center">

```
┌─────────────────────────────────────────────────────────────────┐
│  ⚠️  THIS TOOL EXECUTES REAL ATTACKS AGAINST REAL HARDWARE       │
│                                                                   │
│  Deauth frames are real 802.11 management packets.               │
│  Captive portals harvest real credentials.                        │
│  BLE spam floods real Bluetooth stacks.                          │
│  IR replay controls real TVs.                                    │
│  Sub-GHz replay triggers real RF devices.                        │
│                                                                   │
│  USING THIS ON NETWORKS OR DEVICES YOU DO NOT OWN                │
│  OR HAVE WRITTEN AUTHORIZATION TO TEST IS ILLEGAL.               │
└─────────────────────────────────────────────────────────────────┘
```

</div>

### Legal Requirements

- ✅ You **must** have **explicit, written authorization** from the system owner
- ✅ Use **only** on sandboxed, lab, or personally-owned equipment
- ✅ Run inside an **isoled RF environment** (Faraday cage/bag) when testing Sub-GHz
- ❌ **Never** use on production, corporate, or third-party networks without consent
- ❌ **Never** use for harassment, stalking, or unauthorized surveillance

### Applicable Laws

Violations may be prosecuted under:
- **CFAA** (Computer Fraud and Abuse Act) — US
- **GDPR** Art. 32 — EU (data protection failure)
- **Loi Godfrain** — France (Articles 323-1 to 323-7, Code Pénal)
- **Computer Misuse Act 1990** — UK

> The authors accept **zero liability** for misuse. You are solely responsible for ensuring your use complies with all applicable laws in your jurisdiction.

---

## 📜 Tech Stack

<div align="center">

| Layer | Stack |
|---|---|
| **MCU** | ESP32 WROOM (dual-core Xtensa LX6) |
| **Framework** | Arduino (core 3.3.7) |
| **Language** | C++17 |
| **WiFi Stack** | `esp_wifi_80211_tx` raw frame injection |
| **BLE Stack** | ESP-IDF NimBLE (via Arduino wrappers) |
| **IR Decoder** | IRremoteESP8266 v4.x |
| **JSON Serialization** | ArduinoJson 7.4.3 |
| **Display Driver** | LiquidCrystal_I2C (PCF8574) |
| **Parition Scheme** | `huge_app` (3MB app / 1MB SPIFFS) |

</div>

---

## 📄 License

**MIT** — do whatever you want, just don't blame us.

See [LICENSE](LICENSE) for the full text.

---

<br>

<div align="center">

```
┌──────────────────────────────────────────────────────────────────┐
│  Built for red teams. Use responsibly. Stay curious. Stay legal. │
│                                                                    │
│  ── Bigbrother224                                                 │
└──────────────────────────────────────────────────────────────────┘
```

</div>