// ── Flipper Zero DIY v9 ──
// ESP32 WROOM + CC1101 Sub-GHz + IR + LCD 16x2
// Modular architecture with hardware menu

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

#include "hardware.h"
#include "wifi_attack.h"
#include "ble_spam.h"
#include "captive_portal.h"
#include "wifi_manager.h"
#include "proxy.h"
#include "oled_menu.h"
#include "ir_control.h"
#include "subghz.h"

// ── Feature flags ──
#define ENABLE_BLE_SPAM
#define ENABLE_CAPTIVE_PORTAL

// ── Globals ──
WebServer server(80);
DNSServer dnsServer;
bool bleSpamEnabled = false;
bool deauthEnabled = false;
bool portalEnabled = false;
bool proxyEnabled = false;

// ── Menu definitions ──
static void actionWiFiScan() { performScan(); showStatus("Scan done", String(scanCount) + " networks"); }
static void actionDeauthStart() { startDeauthAttack(); deauthEnabled = true; showStatus("Deauth", "Running"); }
static void actionDeauthStop() { stopAttack(); deauthEnabled = false; showStatus("Deauth", "Stopped"); }
static void actionBLEToggle() { if (!bleSpamActive) { startBLESpam(); bleSpamEnabled = true; } else { stopBLESpam(); bleSpamEnabled = false; } showStatus("BLE Spam", bleSpamActive ? "ON" : "OFF"); }
static void actionSubGHz433() { startSubGHzScan(433.92); showStatus("Sub-GHz", "433MHz scanning"); }
static void actionSubGHz868() { startSubGHzScan(868.00); showStatus("Sub-GHz", "868MHz scanning"); }
static void actionSubGHzStop() { stopSubGHzScan(); showStatus("Sub-GHz", "Stopped"); }
static void actionIRCapture() { startIRCapture(); showStatus("IR", "Capturing..."); }
static void actionIRStop() { stopIRCapture(); showStatus("IR", "Stopped"); }
static void actionIRReplay() { replayLastIR(); showStatus("IR", "Replaying"); }
static void actionProxyToggle() { if (!proxyRunning) { startProxy(proxyPort); proxyEnabled = true; } else { stopProxy(); proxyEnabled = false; } showStatus("Proxy", proxyRunning ? "ON" : "OFF"); }

const MenuItem wifiItems[] = {
  {"Scanner",       MENU_ACTION, (void*)actionWiFiScan},
  {"Deauth Start",  MENU_ACTION, (void*)actionDeauthStart},
  {"Deauth Stop",   MENU_ACTION, (void*)actionDeauthStop},
  {"Portal Start",  MENU_ACTION, (void*)actionStartPortal},
  {"Portal Stop",   MENU_ACTION, (void*)actionStopPortal},
  {"< Back",        MENU_BACK,   nullptr}
};
const MenuScreen menuWiFi = {"WiFi", wifiItems, 6};

const MenuItem bleItems[] = {
  {"Spam",     MENU_TOGGLE, &bleSpamEnabled},
  {"< Back",   MENU_BACK,   nullptr}
};
const MenuScreen menuBLE = {"BLE", bleItems, 2};

const MenuItem subghzItems[] = {
  {"Scan 433MHz",  MENU_ACTION, (void*)actionSubGHz433},
  {"Scan 868MHz",  MENU_ACTION, (void*)actionSubGHz868},
  {"Stop Scan",    MENU_ACTION, (void*)actionSubGHzStop},
  {"Captured",     MENU_VALUE,  nullptr, "0 signals"},
  {"< Back",       MENU_BACK,   nullptr}
};
const MenuScreen menuSubGHz = {"Sub-GHz", subghzItems, 5};

const MenuItem irItems[] = {
  {"Capture",  MENU_ACTION, (void*)actionIRCapture},
  {"Stop",      MENU_ACTION, (void*)actionIRStop},
  {"Replay",    MENU_ACTION, (void*)actionIRReplay},
  {"TV Codes",  MENU_SUBMENU, nullptr},
  {"< Back",    MENU_BACK,   nullptr}
};
const MenuScreen menuIR = {"IR", irItems, 5};

const MenuItem mainItems[] = {
  {"WiFi",     MENU_SUBMENU, (void*)&menuWiFi},
  {"BLE",      MENU_SUBMENU, (void*)&menuBLE},
  {"Sub-GHz",  MENU_SUBMENU, (void*)&menuSubGHz},
  {"IR",       MENU_SUBMENU, (void*)&menuIR},
  {"Proxy",    MENU_TOGGLE,  nullptr, "OFF"},
  {"< Back",   MENU_BACK,    nullptr}
};
const MenuScreen menuMain = {"FLIPPER v9", mainItems, 6};

// ── Portal actions ──
static void actionStartPortal() {
  startCaptivePortal(0, "");
  portalEnabled = true;
  showStatus("Portal", "Running");
}
static void actionStopPortal() {
  stopCaptivePortal();
  portalEnabled = false;
  showStatus("Portal", "Stopped");
}

// ── Web routes ──
void setupWebRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/attack", HTTP_GET, handleAttack);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/target", HTTP_GET, handleTarget);
  server.on("/ble/start", HTTP_GET, [](){ startBLESpam(); bleSpamEnabled = true; server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/ble/stop", HTTP_GET, [](){ stopBLESpam(); bleSpamEnabled = false; server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/portal/start", HTTP_GET, [](){ int t = server.hasArg("t") ? server.arg("t").toInt() : 0; startCaptivePortal(t, ""); portalEnabled = true; server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/portal/stop", HTTP_GET, [](){ stopCaptivePortal(); portalEnabled = false; server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/portal/status", HTTP_GET, [](){ server.send(200, "application/json", getPortalStatusJSON()); });
  server.on("/portal/creds", HTTP_GET, [](){ server.send(200, "application/json", getPortalCredsJSON()); });
  server.on("/portal/templates", HTTP_GET, [](){ server.send(200, "application/json", getPortalTemplatesJSON()); });
  server.on("/portal/clear", HTTP_GET, [](){ clearCreds(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/wifi/scan", HTTP_GET, [](){ server.send(200, "application/json", getWiFiScanJSON()); });
  server.on("/wifi/connect", HTTP_POST, handleWiFiConnect);
  server.on("/wifi/status", HTTP_GET, [](){ server.send(200, "application/json", getWiFiStatusJSON()); });
  server.on("/wifi/forget", HTTP_GET, [](){ forgetWiFi(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/proxy/start", HTTP_GET, [](){ uint16_t p = server.hasArg("port") ? server.arg("port").toInt() : 8080; startProxy(p); proxyEnabled = true; server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/proxy/stop", HTTP_GET, [](){ stopProxy(); proxyEnabled = false; server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/proxy/status", HTTP_GET, [](){ server.send(200, "application/json", getProxyStatusJSON()); });
  server.on("/proxy/log", HTTP_GET, [](){ server.send(200, "application/json", getProxyLogJSON()); });
  server.on("/ir/status", HTTP_GET, [](){ server.send(200, "application/json", getIRStatusJSON()); });
  server.on("/ir/capture", HTTP_GET, [](){ startIRCapture(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/ir/stop", HTTP_GET, [](){ stopIRCapture(); server.send(200, "application/json", getIRCapturedJSON()); });
  server.on("/ir/replay", HTTP_GET, [](){ replayLastIR(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/subghz/status", HTTP_GET, [](){ server.send(200, "application/json", getSubGHzStatusJSON()); });
  server.on("/subghz/scan", HTTP_GET, [](){ float f = server.hasArg("freq") ? server.arg("freq").toFloat() : 433.92; startSubGHzScan(f); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/subghz/stop", HTTP_GET, [](){ stopSubGHzScan(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.onNotFound([](){
    if (portalActive) {
      server.send(200, "text/html", generatePortalHTML(0));
    } else {
      server.sendHeader("Location", "http://192.168.4.1/");
      server.send(302, "text/plain", "");
    }
  });
}

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  JsonDocument doc;
  doc["attack"] = getAttackStatus();
  doc["bleSpam"] = bleSpamActive;
  doc["portal"] = portalActive;
  doc["portalCreds"] = capturedCount;
  doc["proxy"] = proxyRunning;
  doc["heap"] = ESP.getFreeHeap() / 1024;
  doc["irCapturing"] = irCapturing;
  doc["subghzScanning"] = subghzScanning;
  doc["subghzFreq"] = subghzFreq;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleScan() {
  performScan();
  server.send(200, "application/json", getScanJSON());
}

void handleAttack() {
  if (server.hasArg("target")) {
    targetBSSID = server.arg("target");
    if (server.hasArg("ch")) targetChannel = server.arg("ch").toInt();
    startTargetedDeauth();
  } else {
    startDeauthAttack();
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleStop() {
  stopAttack();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleTarget() {
  if (server.hasArg("bssid")) targetBSSID = server.arg("bssid");
  if (server.hasArg("ch")) targetChannel = server.arg("ch").toInt();
  if (server.hasArg("ssid")) targetSSID = server.arg("ssid");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleWiFiConnect() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    saveWiFiCredentials(server.arg("ssid").c_str(), server.arg("pass").c_str());
    WiFi.begin(server.arg("ssid").c_str(), server.arg("pass").c_str());
    server.send(200, "application/json", "{\"ok\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"missing params\"}");
  }
}

// ── Dashboard HTML (embedded) ──
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset=UTF-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>FlipperPro v9</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:#0a0a0f;--card:#16161e;--text:#e2e8f0;--muted:#94a3b8;--accent:#00d4aa;--violet:#6366f1;--red:#ef4444;--orange:#f59e0b;--green:#22c55e;--blue:#3b82f6;--border:#2a2a3a}
body{background:var(--bg);color:var(--text);font-family:system-ui,sans-serif;max-width:480px;margin:0 auto;padding:16px}
h1{font-size:1.4rem;text-align:center;margin:12px 0}
.card{background:var(--card);border:1px solid var(--border);border-radius:14px;padding:14px;margin-bottom:10px}
.card h3{font-size:.9rem;margin-bottom:8px;color:var(--accent)}
.row{display:flex;justify-content:space-between;padding:4px 0;font-size:.82rem;color:var(--muted)}
.row span:last-child{color:var(--text);font-weight:600}
.btn{display:block;width:100%;padding:10px;border:none;border-radius:10px;font-size:.9rem;font-weight:600;cursor:pointer;margin-top:6px;transition:opacity .15s}
.btn:hover{opacity:.85}
.btn-green{background:var(--green);color:#fff}
.btn-red{background:var(--red);color:#fff}
.btn-blue{background:var(--blue);color:#fff}
.btn-violet{background:var(--violet);color:#fff}
.btn-orange{background:var(--orange);color:#fff}
.btn-dark{background:var(--border);color:var(--text)}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:6px}
.grid .btn{margin-top:0}
#status{font-size:.75rem;color:var(--muted);text-align:center;margin-top:12px}
</style></head><body>
<h1>FlipperPro v9</h1>

<div class=card id=attackCard>
<h3>WiFi</h3>
<div class=row><span>Status</span><span id=attackStatus>Idle</span></div>
<div class=row><span>Heap</span><span id=heap>—</span></div>
<div class=grid>
<button class="btn btn-red" onclick="fetch('/attack')">Deauth All</button>
<button class="btn btn-red" onclick="fetch('/stop')">Stop</button>
</div>
<div class=grid style=margin-top:6px>
<button class="btn btn-orange" onclick="fetch('/ble/start')">BLE Spam</button>
<button class="btn btn-dark" onclick="fetch('/ble/stop')">BLE Stop</button>
</div>
</div>

<div class=card>
<h3>Sub-GHz</h3>
<div class=row><span>Status</span><span id=subghzStatus>Idle</span></div>
<div class=row><span>Frequency</span><span id=subghzFreq>—</span></div>
<div class=grid>
<button class="btn btn-violet" onclick="fetch('/subghz/scan?freq=433.92')">433 MHz</button>
<button class="btn btn-violet" onclick="fetch('/subghz/scan?freq=868')">868 MHz</button>
</div>
<button class="btn btn-dark" onclick="fetch('/subghz/stop')">Stop Scan</button>
</div>

<div class=card>
<h3>IR</h3>
<div class=row><span>Status</span><span id=irStatus>Idle</span></div>
<div class=grid>
<button class="btn btn-orange" onclick="fetch('/ir/capture')">Capture</button>
<button class="btn btn-dark" onclick="fetch('/ir/stop')">Stop</button>
</div>
<button class="btn btn-blue" onclick="fetch('/ir/replay')">Replay Last</button>
</div>

<div class=card>
<h3>Captive Portal</h3>
<div class=row><span>Credentials</span><span id=credCount>0</span></div>
<div class=grid>
<button class="btn btn-orange" onclick="fetch('/portal/start?t=0')">Start</button>
<button class="btn btn-dark" onclick="fetch('/portal/stop')">Stop</button>
</div>
<button class="btn btn-dark" onclick="fetch('/portal/clear')">Clear Creds</button>
</div>

<div class=card>
<h3>Proxy</h3>
<div class=row><span>Running</span><span id=proxyStatus>No</span></div>
<button class="btn btn-green" onclick="fetch('/proxy/start?port=8080')">Start (8080)</button>
<button class="btn btn-dark" onclick="fetch('/proxy/stop')">Stop</button>
</div>

<div id=status>Refreshing...</div>

<script>
setInterval(async()=>{
  try{const r=await fetch('/status');const d=await r.json();
  document.getElementById('attackStatus').textContent=d.attack==='idle'?'Idle':d.attack==='broadcast'?'Broadcast':'Targeted';
  document.getElementById('heap').textContent=d.heap+'KB';
  document.getElementById('credCount').textContent=d.portalCreds;
  document.getElementById('proxyStatus').textContent=d.proxy?'Yes':'No';
  document.getElementById('subghzStatus').textContent=d.subghzScanning?'Scanning':'Idle';
  document.getElementById('subghzFreq').textContent=d.subghzScanning?d.subghzFreq+'MHz':'—';
  document.getElementById('irStatus').textContent=d.irCapturing?'Capturing':'Idle';
  }catch(e){document.getElementById('status').textContent='Error: '+e.message}
},2000);
</script></body></html>
)rawliteral";

// ── setup() ──
void setup() {
  Serial.begin(115200);
  Serial.println("FlipperPro v9 starting...");

  // WDT
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  // Init modules
  initLCD();
  initWiFiAttack();
  initBLESpam();
  initCaptivePortal();
  initWiFiManager();
  initProxy();
  initIR();
  initSubGHz();

  // WiFi AP
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("AP started: " AP_SSID);

  // Connect to saved WiFi
  connectToSavedWiFi();

  // mDNS
  MDNS.begin("flipper");

  // Web server
  setupWebRoutes();
  server.begin();

  // DNS for captive portal
  dnsServer.start(53, "*", WiFi.softAPIP());

  showStatus("FlipperPro v9", "Ready");
  Serial.println("Setup complete.");
}

// ── loop() ──
void loop() {
  esp_task_wdt_reset();

  // DNS
  dnsServer.processNextRequest();

  // HTTP
  server.handleClient();

  // WiFi reconnect
  checkWiFiReconnect();

  // BLE spam
  updateBLESpam();

  // Deauth attack
  wifiAttackLoop();

  // Proxy
  proxyLoop();

  // IR capture
  irLoop();

  // Sub-GHz scan
  subghzLoop();

  // LCD menu
  updateLCD();
}