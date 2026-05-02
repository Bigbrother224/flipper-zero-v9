#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ── LCD 16x2 I2C Menu Module ──
// Replaces OLED — much simpler, less RAM

enum MenuItemType {
  MENU_SUBMENU,
  MENU_ACTION,
  MENU_TOGGLE,
  MENU_VALUE,
  MENU_BACK
};

struct MenuItem {
  const char* label;
  MenuItemType type;
  void* data;
  const char* value;
};

struct MenuScreen {
  const char* title;
  const MenuItem* items;
  int itemCount;
};

void initLCD();
void updateLCD();
void drawMenu();
void handleButtonUP();
void handleButtonDown();
void handleButtonOK();
void handleButtonBack();
void showStatus(const char* line1, const char* line2 = nullptr, int duration = 2000);

extern LiquidCrystal_I2C lcd;
extern int menuCursor;
extern const MenuScreen* currentMenu;
extern const MenuScreen* previousMenu[10];
extern int menuDepth;