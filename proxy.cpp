#include "hardware.h"
#include "proxy.h"
#include <WiFi.h>
#include <ArduinoJson.h>

bool proxyRunning = false;
uint16_t proxyPort = 8080;
uint32_t proxyTotalIn = 0;
uint32_t proxyTotalOut = 0;
int proxyActiveClients = 0;
ProxyLogEntry proxyLog[PROXY_LOG_SIZE];
int proxyLogCount = 0;
static WiFiServer *proxyServer = nullptr;
static WiFiClient proxyClients[MAX_PROXY_CLIENTS];

void initProxy() {
  proxyRunning = false;
  proxyLogCount = 0;
}

bool startProxy(uint16_t port) {
  if (proxyRunning) stopProxy();
  if (port < 1) port = 8080;
  proxyPort = port;
  proxyServer = new WiFiServer(proxyPort);
  proxyServer->begin();
  proxyRunning = true;
  proxyTotalIn = 0;
  proxyTotalOut = 0;
  return true;
}

void stopProxy() {
  if (proxyServer) {
    proxyServer->stop();
    delete proxyServer;
    proxyServer = nullptr;
  }
  for (int i = 0; i < MAX_PROXY_CLIENTS; i++) {
    if (proxyClients[i]) proxyClients[i].stop();
  }
  proxyRunning = false;
  proxyActiveClients = 0;
}

String getProxyStatusJSON() {
  JsonDocument doc;
  doc["running"] = proxyRunning;
  doc["port"] = proxyPort;
  doc["bytesIn"] = proxyTotalIn;
  doc["bytesOut"] = proxyTotalOut;
  doc["clients"] = proxyActiveClients;
  String out;
  serializeJson(doc, out);
  return out;
}

static void proxyAddLog(const char* method, const char* host, uint16_t port, uint32_t in, uint32_t out) {
  if (proxyLogCount >= PROXY_LOG_SIZE) {
    // Shift buffer left (drop oldest)
    memmove(&proxyLog[0], &proxyLog[1], (PROXY_LOG_SIZE - 1) * sizeof(ProxyLogEntry));
    proxyLogCount = PROXY_LOG_SIZE - 1;
  }
  ProxyLogEntry &e = proxyLog[proxyLogCount++];
  e.timestamp = millis();
  strncpy(e.method, method, 7);
  e.method[7] = '\0';
  strncpy(e.host, host, 47);
  e.host[47] = '\0';
  e.port = port;
  e.bytesIn = in;
  e.bytesOut = out;
}

String getProxyLogJSON() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < proxyLogCount; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["ts"] = proxyLog[i].timestamp;
    obj["method"] = proxyLog[i].method;
    obj["host"] = proxyLog[i].host;
    obj["port"] = proxyLog[i].port;
    obj["in"] = proxyLog[i].bytesIn;
    obj["out"] = proxyLog[i].bytesOut;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

void proxyLoop() {
  if (!proxyRunning || !proxyServer) return;

  // Accept new clients
  WiFiClient newClient = proxyServer->available();
  if (newClient) {
    for (int i = 0; i < MAX_PROXY_CLIENTS; i++) {
      if (!proxyClients[i] || !proxyClients[i].connected()) {
        proxyClients[i] = newClient;
        proxyActiveClients++;
        break;
      }
    }
  }

  // Relay data per client
  for (int i = 0; i < MAX_PROXY_CLIENTS; i++) {
    if (!proxyClients[i] || !proxyClients[i].connected()) {
      if (proxyClients[i] && proxyActiveClients > 0) proxyActiveClients--;
      continue;
    }

    uint8_t buf[PROXY_BUF_SIZE];
    while (proxyClients[i].available()) {
      int len = proxyClients[i].read(buf, sizeof(buf));
      if (len > 0) {
        proxyTotalIn += len;
        proxyAddLog("RELAY", "tcp", proxyPort, len, 0);
      }
    }
  }
}
