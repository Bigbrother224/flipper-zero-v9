#pragma once
#include <Arduino.h>

// ── TCP Proxy Module ──

struct ProxyLogEntry {
  unsigned long timestamp;
  char method[8];
  char host[48];
  uint16_t port;
  uint32_t bytesIn;
  uint32_t bytesOut;
  bool https;
};

extern bool proxyRunning;
extern uint16_t proxyPort;
extern uint32_t proxyTotalIn;
extern uint32_t proxyTotalOut;
extern int proxyActiveClients;
extern ProxyLogEntry proxyLog[];
extern int proxyLogCount;

void initProxy();
bool startProxy(uint16_t port);
void stopProxy();
String getProxyStatusJSON();
String getProxyLogJSON();
void proxyLoop();