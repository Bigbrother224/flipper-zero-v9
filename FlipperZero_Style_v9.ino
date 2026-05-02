// ── Reaper v9 ──
// ESP32 WROOM + CC1101 Sub-GHz + IR + LCD 16x2
// Modular offensive security multi-tool

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

// ── JSON helper ──
static void sendJSON(int code, const char* json) {
  server.send(code, "application/json", json);
}

// ── Menu action wrappers ──
static void actionWiFiScan() { performScan(); showStatus("Scan done", String(scanCount) + " nets"); }
static void actionDeauthStart() { startDeauthAttack(); deauthEnabled = true; showStatus("Deauth", "Running"); }
static void actionDeauthStop() { stopAttack(); deauthEnabled = false; showStatus("Deauth", "Stopped"); }
static void actionBLEToggle() { if (!bleSpamActive) { startBLESpam(); } else { stopBLESpam(); } bleSpamEnabled = bleSpamActive; showStatus("BLE", bleSpamActive ? "ON" : "OFF"); }
static void actionSubGHz433() { startSubGHzScan(433.92); showStatus("Sub-GHz", "433MHz"); }
static void actionSubGHz868() { startSubGHzScan(868.00); showStatus("Sub-GHz", "868MHz"); }
static void actionSubGHzStop() { stopSubGHzScan(); showStatus("Sub-GHz", "Stopped"); }
static void actionIRCapture() { startIRCapture(); showStatus("IR", "Capturing..."); }
static void actionIRStop() { stopIRCapture(); showStatus("IR", "Stopped"); }
static void actionIRReplay() { replayLastIR(); showStatus("IR", "Replaying"); }
static void actionProxyToggle() { if (!proxyRunning) { startProxy(proxyPort); } else { stopProxy(); } proxyEnabled = proxyRunning; showStatus("Proxy", proxyRunning ? "ON" : "OFF"); }

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
  {"Spam",     MENU_ACTION, (void*)actionBLEToggle},
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
  {"Stop",     MENU_ACTION, (void*)actionIRStop},
  {"Replay",   MENU_ACTION, (void*)actionIRReplay},
  {"TV Codes", MENU_SUBMENU, nullptr},
  {"< Back",   MENU_BACK,   nullptr}
};
const MenuScreen menuIR = {"IR", irItems, 5};

const MenuItem mainItems[] = {
  {"WiFi",     MENU_SUBMENU, (void*)&menuWiFi},
  {"BLE",      MENU_SUBMENU, (void*)&menuBLE},
  {"Sub-GHz",  MENU_SUBMENU, (void*)&menuSubGHz},
  {"IR",       MENU_SUBMENU, (void*)&menuIR},
  {"Proxy",    MENU_ACTION,  (void*)actionProxyToggle},
  {"About",    MENU_VALUE,   nullptr, "Reaper v9"},
  {"< Back",   MENU_BACK,    nullptr}
};
const MenuScreen menuMain = {"REAPER v9", mainItems, 7};

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

  server.on("/ble/start", HTTP_GET, [](){ startBLESpam(); bleSpamEnabled = true; sendJSON(200, "{\"ok\":true}"); });
  server.on("/ble/stop", HTTP_GET, [](){ stopBLESpam(); bleSpamEnabled = false; sendJSON(200, "{\"ok\":true}"); });

  server.on("/portal/start", HTTP_GET, [](){
    int t = server.hasArg("t") ? server.arg("t").toInt() : 0;
    if (t < 0 || t >= NUM_PORTAL_STYLES) t = 0;
    startCaptivePortal(t, "");
    portalEnabled = true;
    sendJSON(200, "{\"ok\":true}");
  });
  server.on("/portal/stop", HTTP_GET, [](){ stopCaptivePortal(); portalEnabled = false; sendJSON(200, "{\"ok\":true}"); });
  server.on("/portal/status", HTTP_GET, [](){ sendJSON(200, getPortalStatusJSON()); });
  server.on("/portal/creds", HTTP_GET, [](){ sendJSON(200, getPortalCredsJSON()); });
  server.on("/portal/templates", HTTP_GET, [](){ sendJSON(200, getPortalTemplatesJSON()); });
  server.on("/portal/clear", HTTP_GET, [](){ clearCreds(); sendJSON(200, "{\"ok\":true}"); });

  server.on("/wifi/scan", HTTP_GET, [](){ sendJSON(200, getWiFiScanJSON()); });
  server.on("/wifi/connect", HTTP_POST, handleWiFiConnect);
  server.on("/wifi/status", HTTP_GET, [](){ sendJSON(200, getWiFiStatusJSON()); });
  server.on("/wifi/forget", HTTP_GET, [](){ forgetWiFi(); sendJSON(200, "{\"ok\":true}"); });

  server.on("/proxy/start", HTTP_GET, [](){
    uint16_t port = server.hasArg("port") ? server.arg("port").toInt() : 8080;
    if (port < 1 || port > 65535) port = 8080;
    startProxy(port);
    proxyEnabled = true;
    sendJSON(200, "{\"ok\":true}");
  });
  server.on("/proxy/stop", HTTP_GET, [](){ stopProxy(); proxyEnabled = false; sendJSON(200, "{\"ok\":true}"); });
  server.on("/proxy/status", HTTP_GET, [](){ sendJSON(200, getProxyStatusJSON()); });
  server.on("/proxy/log", HTTP_GET, [](){ sendJSON(200, getProxyLogJSON()); });

  server.on("/ir/status", HTTP_GET, [](){ sendJSON(200, getIRStatusJSON()); });
  server.on("/ir/capture", HTTP_GET, [](){ startIRCapture(); sendJSON(200, "{\"ok\":true}"); });
  server.on("/ir/stop", HTTP_GET, [](){ stopIRCapture(); sendJSON(200, getIRCapturedJSON()); });
  server.on("/ir/replay", HTTP_GET, [](){ replayLastIR(); sendJSON(200, "{\"ok\":true}"); });
  server.on("/ir/bruteforce/tv", HTTP_GET, [](){
    String brand = server.hasArg("brand") ? server.arg("brand") : "all";
    bruteForceTV(brand.c_str());
    sendJSON(200, "{\"ok\":true}");
  });

  server.on("/subghz/status", HTTP_GET, [](){ sendJSON(200, getSubGHzStatusJSON()); });
  server.on("/subghz/scan", HTTP_GET, [](){
    float freq = server.hasArg("freq") ? server.arg("freq").toFloat() : 433.92;
    if (freq < 300.0 || freq > 928.0) freq = 433.92;
    startSubGHzScan(freq);
    sendJSON(200, "{\"ok\":true}");
  });
  server.on("/subghz/stop", HTTP_GET, [](){ stopSubGHzScan(); sendJSON(200, "{\"ok\":true}"); });

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
  doc["irCount"] = irCapturedCount;
  doc["subghzScanning"] = subghzScanning;
  doc["subghzFreq"] = subghzFreq;
  doc["cc1101"] = cc1101Present;
  String resp;
  serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void handleScan() {
  performScan();
  server.send(200, "application/json", getScanJSON());
}

void handleAttack() {
  if (server.hasArg("target")) {
    String tgt = server.arg("target");
    if (tgt.length() == 17) {  // basic MAC validation
      targetBSSID = tgt;
      if (server.hasArg("ch")) {
        int ch = server.arg("ch").toInt();
        if (ch >= 1 && ch <= 13) targetChannel = ch;
      }
      startTargetedDeauth();
    } else {
      sendJSON(400, "{\"error\":\"invalid mac format\"}");
      return;
    }
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
  if (server.hasArg("ch")) {
    int ch = server.arg("ch").toInt();
    if (ch >= 1 && ch <= 13) targetChannel = ch;
  }
  if (server.hasArg("ssid")) targetSSID = server.arg("ssid");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleWiFiConnect() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    if (ssid.length() > 0 && ssid.length() <= 32) {
      saveWiFiCredentials(ssid.c_str(), pass.c_str());
      WiFi.begin(ssid.c_str(), pass.c_str());
      sendJSON(200, "{\"ok\":true}");
      return;
    }
  }
  sendJSON(400, "{\"error\":\"missing or invalid params\"}");
}

// ── Dashboard HTML (embedded) ──
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Reaper v9</title>
<style>
:root{
  --bg:#06060e;
  --card:#12121c;
  --text:#e4e8f0;
  --muted:#8892a8;
  --accent:#0ff;
  --violet:#818cf8;
  --red:#f43f5e;
  --orange:#fb923c;
  --green:#4ade80;
  --blue:#60a5fa;
  --border:#1e1e32;
  --glow:0 0 20px rgba(0,255,255,.06);
}
*{margin:0;padding:0;box-sizing:border-box}
body{
  background:var(--bg);
  background-image:
    radial-gradient(ellipse at 20% 5%,rgba(0,255,255,.04),transparent 70%),
    radial-gradient(ellipse at 80% 95%,rgba(129,140,248,.03),transparent 70%);
  color:var(--text);
  font-family:'SF Pro',system-ui,-apple-system,sans-serif;
  max-width:500px;
  margin:0 auto;
  padding:16px;
  min-height:100vh;
}
h1{
  font-size:1.6rem;
  text-align:center;
  margin:16px 0 4px;
  letter-spacing:2px;
  color:var(--accent);
  text-shadow:0 0 30px rgba(0,255,255,.3);
}
.sub{
  text-align:center;
  font-size:.75rem;
  color:var(--muted);
  margin-bottom:16px;
  text-transform:uppercase;
  letter-spacing:3px;
}
.card{
  background:var(--card);
  border:1px solid var(--border);
  border-radius:16px;
  padding:16px;
  margin-bottom:10px;
  box-shadow:var(--glow);
}
.card h3{
  font-size:.85rem;
  margin-bottom:10px;
  color:var(--accent);
  letter-spacing:1px;
  text-transform:uppercase;
  font-weight:500;
}
.row{
  display:flex;
  justify-content:space-between;
  padding:4px 0;
  font-size:.8rem;
  color:var(--muted);
  font-family:'JetBrains Mono','SF Mono',monospace;
}
.row span:last-child{
  color:var(--text);
  font-weight:500
}
.btn{
  display:block;
  width:100%;
  padding:11px;
  border:none;
  border-radius:12px;
  font-size:.85rem;
  font-weight:600;
  cursor:pointer;
  margin-top:7px;
  font-family:system-ui,sans-serif;
  font-weight:500;
  letter-spacing:0.5px;
  transition:all .15s;
}
.btn:hover{filter:brightness(1.2);transform:translateY(-1px)}
.btn-red,.btn-attack{background:var(--red);color:#fff}
.btn-green,.btn-start{background:var(--green);color:#000}
.btn-blue,.btn-primary{background:var(--blue);color:#000}
.btn-violet,.btn-scan{background:var(--violet);color:#fff}
.btn-orange,.btn-capture{background:var(--orange);color:#000}
.btn-dark,.btn-secondary{background:var(--border);color:var(--text)}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:7px}
.grid .btn{margin-top:0}
.status-bar{
  font-size:.7rem;
  color:var(--muted);
  text-align:center;
  margin-top:14px;
  font-family:'JetBrains Mono','SF Mono',monospace;
  padding:8px;
  border-top:1px solid var(--border);
}
.status-bar span{color:var(--accent)}
</style>
</head>
<body>

<h1>REAPER</h1>
<div class="sub">ESP32 Security Toolkit v9</div>

<div class="card">
<h3>&#9767; WiFi Attack</h3>
<div class="row"><span>Status</span><span id="attackStatus">Idle</span></div>
<div class="row"><span>Target</span><span id="attackTarget">—</span></div>
<div class="row"><span>Heap</span><span id="heap">—</span></div>
<div class="grid">
<button class="btn btn-attack" onclick="call('/attack')">Deauth All</button>
<button class="btn btn-attack" onclick="call('/stop')">Stop</button>
</div>
<div class="grid" style="margin-top:7px">
<button class="btn btn-capture" onclick="call('/ble/start')">BLE Spam</button>
<button class="btn btn-secondary" onclick="call('/ble/stop')">BLE Stop</button>
</div>
</div>

<div class="card">
<h3>&#10022; Sub-GHz Radio</h3>
<div class="row"><span>Status</span><span id="subghzStatus">Idle</span></div>
<div class="row"><span>Frequency</span><span id="subghzFreq">—</span></div>
<div class="row"><span>Hardware</span><span id="cc1101Status">—</span></div>
<div class="grid">
<button class="btn btn-scan" onclick="call('/subghz/scan?freq=433.92')">433 MHz</button>
<button class="btn btn-scan" onclick="call('/subghz/scan?freq=868')">868 MHz</button>
</div>
<button class="btn btn-secondary" onclick="call('/subghz/stop')">Stop Scan</button>
</div>

<div class="card">
<h3>&#9681; IR Control</h3>
<div class="row"><span>Status</span><span id="irStatus">Idle</span></div>
<div class="row"><span>Captured</span><span id="irCount">0</span></div>
<div class="grid">
<button class="btn btn-capture" onclick="call('/ir/capture')">Capture</button>
<button class="btn btn-secondary" onclick="call('/ir/stop')">Stop</button>
</div>
<button class="btn btn-primary" onclick="call('/ir/replay')">Replay Last</button>
</div>

<div class="card">
<h3>&#9881; Captive Portal</h3>
<div class="row"><span>Credentials</span><span id="credCount">0</span></div>
<div class="grid">
<button class="btn btn-capture" onclick="call('/portal/start?t=0')">Start</button>
<button class="btn btn-secondary" onclick="call('/portal/stop')">Stop</button>
</div>
<button class="btn btn-secondary" onclick="call('/portal/clear')">Clear Creds</button>
</div>

<div class="card">
<h3>&#9788; TCP Proxy</h3>
<div class="row"><span>Running</span><span id="proxyStatus">No</span></div>
<div class="row"><span>Port</span><span id="proxyPort">—</span></div>
<button class="btn btn-start" onclick="call('/proxy/start?port=8080')">Start Proxy :8080</button>
<button class="btn btn-secondary" onclick="call('/proxy/stop')">Stop Proxy</button>
</div>

<div class="status-bar">
<span>&#9679;</span> Live · Auto-refresh 2s · <span id="uptime">0s</span> uptime
</div>

<script>
let uptimeSec=0;
setInterval(()=>{uptimeSec++;document.getElementById('uptime').textContent=uptimeSec+'s'},1000);

function call(url){
  fetch(url).then(r=>r.json()).then(d=>console.log(d)).catch(e=>console.error(e))
}

setInterval(async()=>{
  try{
    const r=await fetch('/status');
    const d=await r.json();
    document.getElementById('attackStatus').textContent=d.attack==='idle'?'Idle':d.attack==='broadcast'?'Broadcast':'Targeted';
    document.getElementById('heap').textContent=d.heap+' KB';
    document.getElementById('credCount').textContent=d.portalCreds||0;
    document.getElementById('proxyStatus').textContent=d.proxy?'Yes':'No';
    document.getElementById('subghzStatus').textContent=d.subghzScanning?'Scanning':'Idle';
    document.getElementById('subghzFreq').textContent=d.subghzScanning?d.subghzFreq+' MHz':'—';
    document.getElementById('cc1101Status').textContent=d.cc1101?'CC1101 Ready':'Not Detected';
    document.getElementById('irStatus').textContent=d.irCapturing?'Capturing':'Idle';
    document.getElementById('irCount').textContent=d.irCount||0;
  }catch(e){}
},2000);
</script>
</body></html>)rawliteral";

// ── setup() ──
void setup() {
  Serial.begin(115200);
  Serial.println("\n╔══════════════════════╗");
  Serial.println(  "║   REAPER v9 BOOT    ║");
  Serial.println(  "╚══════════════════════╝");

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
  Serial.printf("AP: %s / %s\n", AP_SSID, AP_PASSWORD);

  // Async connect to saved WiFi
  connectToSavedWiFi();

  // mDNS
  MDNS.begin("reaper");

  // Web server
  setupWebRoutes();
  server.begin();

  // DNS for captive portal
  dnsServer.start(53, "*", WiFi.softAPIP());

  showStatus("REAPER v9", "Ready");
  Serial.println(">>> Reaper online <<<");
}

// ── loop() ──
void loop() {
  esp_task_wdt_reset();

  dnsServer.processNextRequest();
  server.handleClient();

  checkWiFiReconnect();
  updateBLESpam();
  wifiAttackLoop();
  proxyLoop();
  irLoop();
  subghzLoop();
  updateLCD();
}
