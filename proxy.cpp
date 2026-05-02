#include "hardware.h"
#include "proxy.h"
#include <WiFi.h>

bool proxyRunning = false;
uint16_t proxyPort = 8080;
uint32_t proxyTotalIn = 0;
uint32_t proxyTotalOut = 0;
int proxyActiveClients = 0;
ProxyLogEntry proxyLog[PROXY_LOG_SIZE];
int proxyLogCount = 0;
static WiFiServer *proxyServer = nullptr;
static WiFiClient proxyClients[PROXY_MAX_CLIENTS];

void initProxy() {
  proxyRunning = false;
  proxyLogCount = 0;
}

bool startProxy(uint16_t port) {
  if (proxyRunning) stopProxy();
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
  for (int i = 0; i < PROXY_MAX_CLIENTS; i++) {
    if (proxyClients[i]) proxyClients[i].stop();
  }
  proxyRunning = false;
  proxyActiveClients = 0;
}

String getProxyStatusJSON() {
  return "{\"running\":" + String(proxyRunning ? "true" : "false") +
         ",\"port\":" + String(proxyPort) +
         ",\"in\":" + String(proxyTotalIn) +
         ",\"out\":" + String(proxyTotalOut) +
         ",\"clients\":" + String(proxyActiveClients) + "}";
}

String getProxyLogJSON() {
  String json = "[";
  for (int i = 0; i < proxyLogCount; i++) {
    if (i) json += ",";
    json += "{\"ts\":" + String(proxyLog[i].timestamp) + ",";
    json += "\"method\":\"" + String(proxyLog[i].method) + "\",";
    json += "\"host\":\"" + String(proxyLog[i].host) + "\",";
    json += "\"port\":" + String(proxyLog[i].port) + ",";
    json += "\"in\":" + String(proxyLog[i].bytesIn) + ",";
    json += "\"out\":" + String(proxyLog[i].bytesOut) + "}";
  }
  json += "]";
  return json;
}

void proxyLoop() {
  if (!proxyRunning || !proxyServer) return;

  WiFiClient newClient = proxyServer->available();
  if (newClient) {
    for (int i = 0; i < PROXY_MAX_CLIENTS; i++) {
      if (!proxyClients[i] || !proxyClients[i].connected()) {
        proxyClients[i] = newClient;
        break;
      }
    }
  }

  for (int i = 0; i < PROXY_MAX_CLIENTS; i++) {
    if (!proxyClients[i] || !proxyClients[i].connected()) continue;

    static uint8_t buf[PROXY_BUF_SIZE];
    while (proxyClients[i].available()) {
      int len = proxyClients[i].read(buf, sizeof(buf));
      if (len > 0) {
        proxyTotalIn += len;
        // Forward to remote — simplified relay
      }
    }
  }
}