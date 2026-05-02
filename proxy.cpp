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
static WiFiClient proxyRemotes[MAX_PROXY_CLIENTS];
static char proxyTargetHost[64] = "";
static uint16_t proxyTargetPort = 80;

void initProxy() {
  proxyRunning = false;
  proxyLogCount = 0;
  proxyTargetHost[0] = '\0';
  proxyTargetPort = 80;
}

bool startProxy(uint16_t port) {
  if (proxyRunning) stopProxy();
  if (port < 1) port = 8080;
  proxyPort = port;
  if (proxyServer) delete proxyServer;
  proxyServer = new WiFiServer(proxyPort);
  if (!proxyServer) return false;
  proxyServer->begin();
  proxyRunning = true;
  proxyTotalIn = 0;
  proxyTotalOut = 0;
  return true;
}

void stopProxy() {
  for (int i = 0; i < MAX_PROXY_CLIENTS; i++) {
    if (proxyClients[i]) proxyClients[i].stop();
    if (proxyRemotes[i]) proxyRemotes[i].stop();
  }
  if (proxyServer) {
    proxyServer->stop();
    delete proxyServer;
    proxyServer = nullptr;
  }
  proxyRunning = false;
  proxyActiveClients = 0;
}

bool setProxyTarget(const char* host, uint16_t port) {
  if (!host || strlen(host) == 0 || strlen(host) >= sizeof(proxyTargetHost)) return false;
  if (port < 1 || port > 65535) return false;
  strncpy(proxyTargetHost, host, sizeof(proxyTargetHost) - 1);
  proxyTargetHost[sizeof(proxyTargetHost) - 1] = '\0';
  proxyTargetPort = port;
  return true;
}

static void proxyAddLog(const char* method, const char* host, uint16_t port, uint32_t in, uint32_t out) {
  int idx;
  if (proxyLogCount < PROXY_LOG_SIZE) {
    idx = proxyLogCount++;
  } else {
    // Circular buffer: overwrite oldest
    idx = 0;
  }
  ProxyLogEntry &e = proxyLog[idx];
  e.timestamp = millis();
  strncpy(e.method, method, 7);
  e.method[7] = '\0';
  strncpy(e.host, host, 47);
  e.host[47] = '\0';
  e.port = port;
  e.bytesIn = in;
  e.bytesOut = out;
}

String getProxyStatusJSON() {
  JsonDocument doc;
  doc["running"] = proxyRunning;
  doc["port"] = proxyPort;
  doc["bytesIn"] = proxyTotalIn;
  doc["bytesOut"] = proxyTotalOut;
  doc["clients"] = proxyActiveClients;
  doc["targetHost"] = proxyTargetHost;
  doc["targetPort"] = proxyTargetPort;
  String out;
  serializeJson(doc, out);
  return out;
}

String getProxyLogJSON() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < proxyLogCount && i < PROXY_LOG_SIZE; i++) {
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
  uint8_t buf[PROXY_BUF_SIZE];
  for (int i = 0; i < MAX_PROXY_CLIENTS; i++) {
    if (!proxyClients[i] || !proxyClients[i].connected()) {
      if (proxyClients[i]) {
        proxyClients[i].stop();
        if (proxyActiveClients > 0) proxyActiveClients--;
      }
      if (proxyRemotes[i]) proxyRemotes[i].stop();
      continue;
    }

    // Read from client
    if (proxyClients[i].available()) {
      // If no target configured, try to connect
      if (!proxyRemotes[i] || !proxyRemotes[i].connected()) {
        if (proxyTargetHost[0] != '\0') {
          proxyRemotes[i] = WiFiClient();
          if (!proxyRemotes[i].connect(proxyTargetHost, proxyTargetPort, 3000)) {
            proxyAddLog("FAIL", proxyTargetHost, proxyTargetPort, 0, 0);
            proxyClients[i].stop();
            continue;
          }
        } else {
          // No target configured — just count bytes and discard
          int len = proxyClients[i].read(buf, sizeof(buf));
          if (len > 0) {
            proxyTotalIn += len;
            proxyAddLog("DISCARD", "none", 0, len, 0);
          }
          continue;
        }
      }

      // Relay client -> remote
      while (proxyClients[i].available()) {
        int len = proxyClients[i].read(buf, sizeof(buf));
        if (len > 0) {
          proxyRemotes[i].write(buf, len);
          proxyTotalIn += len;
          proxyAddLog("RELAY", proxyTargetHost, proxyTargetPort, len, 0);
        }
      }
    }

    // Read from remote -> client
    if (proxyRemotes[i] && proxyRemotes[i].connected() && proxyRemotes[i].available()) {
      while (proxyRemotes[i].available()) {
        int len = proxyRemotes[i].read(buf, sizeof(buf));
        if (len > 0) {
          proxyClients[i].write(buf, len);
          proxyTotalOut += len;
          proxyAddLog("RELAY", proxyTargetHost, proxyTargetPort, 0, len);
        }
      }
    }
  }
}