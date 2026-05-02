#include "hardware.h"
#include "oled_menu.h"

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
int menuCursor = 0;
const MenuScreen* currentMenu = nullptr;
const MenuScreen* previousMenu[10];
int menuDepth = 0;

static unsigned long lastInputTime = 0;
static const int INPUT_DEBOUNCE = 250;  // ms between inputs
static String statusLine1 = "";
static String statusLine2 = "";
static unsigned long statusTimeout = 0;

// Forward declarations from main .ino
extern const MenuScreen menuMain;

// ── Joystick reading ──

static int readJoyX() { return analogRead(JOY_VRX); }
static int readJoyY() { return analogRead(JOY_VRY); }
static bool readJoySW() { return digitalRead(JOY_SW) == LOW; }

// Returns: 0=none, 1=up, 2=down, 3=left, 4=right, 5=click
static int readJoystick() {
  int x = readJoyX();
  int y = readJoyY();

  // Y axis: up = low value, down = high value
  if (y < JOY_THRESHOLD) return 1;       // UP
  if (y > 4095 - JOY_THRESHOLD) return 2; // DOWN

  // X axis: left = low value, right = high value
  if (x < JOY_THRESHOLD) return 3;       // LEFT (BACK)
  if (x > 4095 - JOY_THRESHOLD) return 4; // RIGHT (OK)

  // Push button
  if (readJoySW()) return 5;             // CLICK (OK)

  return 0;
}

void initLCD() {
  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("FLIPPER v9");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  // Joystick pins
  pinMode(JOY_VRX, INPUT);
  pinMode(JOY_VRY, INPUT);
  pinMode(JOY_SW, INPUT_PULLUP);

  currentMenu = &menuMain;
  menuCursor = 0;
  menuDepth = 0;
}

static bool debounce() {
  if (millis() - lastInputTime < INPUT_DEBOUNCE) return false;
  lastInputTime = millis();
  return true;
}

void handleButtonUP() {
  if (!debounce()) return;
  if (currentMenu && menuCursor > 0) menuCursor--;
}

void handleButtonDown() {
  if (!debounce()) return;
  if (currentMenu && menuCursor < currentMenu->itemCount - 1) menuCursor++;
}

void handleButtonOK() {
  if (!debounce()) return;
  if (!currentMenu) return;
  const MenuItem &item = currentMenu->items[menuCursor];

  switch (item.type) {
    case MENU_SUBMENU:
      if (menuDepth < 10) previousMenu[menuDepth++] = currentMenu;
      currentMenu = (const MenuScreen*)item.data;
      menuCursor = 0;
      break;
    case MENU_TOGGLE:
      if (item.data) *(bool*)item.data = !*(bool*)item.data;
      break;
    case MENU_BACK:
      handleButtonBack();
      break;
    default:
      break;
  }
}

void handleButtonBack() {
  if (!debounce()) return;
  if (menuDepth > 0) {
    currentMenu = previousMenu[--menuDepth];
    menuCursor = 0;
  }
}

void showStatus(const char* l1, const char* l2, int duration) {
  statusLine1 = l1;
  statusLine2 = l2 ? l2 : "";
  statusTimeout = millis() + duration;
}

void drawMenu() {
  // Status override
  if (statusLine1.length() > 0 && (statusTimeout == 0 || millis() < statusTimeout)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(statusLine1.c_str());
    if (statusLine2.length() > 0) {
      lcd.setCursor(0, 1);
      lcd.print(statusLine2.c_str());
    }
    if (statusTimeout > 0 && millis() >= statusTimeout) {
      statusLine1 = "";
      statusLine2 = "";
    }
    return;
  } else {
    statusLine1 = "";
    statusLine2 = "";
  }

  if (!currentMenu) return;

  lcd.clear();

  // Line 0: current item with cursor >
  lcd.setCursor(0, 0);
  lcd.print(">");

  const MenuItem &curItem = currentMenu->items[menuCursor];
  String curLabel = curItem.label;
  if (curItem.type == MENU_TOGGLE && curItem.data) {
    curLabel += *(bool*)curItem.data ? " ON" : " OFF";
  }
  if (curLabel.length() > 15) curLabel = curLabel.substring(0, 15);
  lcd.print(curLabel);

  // Line 1: next item preview
  lcd.setCursor(0, 1);
  if (menuCursor < currentMenu->itemCount - 1) {
    lcd.print(" ");
    const MenuItem &nextItem = currentMenu->items[menuCursor + 1];
    String nextLabel = nextItem.label;
    if (nextItem.type == MENU_TOGGLE && nextItem.data) {
      nextLabel += *(bool*)nextItem.data ? " ON" : " OFF";
    }
    if (nextLabel.length() > 15) nextLabel = nextLabel.substring(0, 15);
    lcd.print(nextLabel);
  } else if (menuDepth > 0) {
    lcd.print("[BACK:");
    String parentTitle = previousMenu[menuDepth - 1]->title;
    if (parentTitle.length() > 9) parentTitle = parentTitle.substring(0, 9);
    lcd.print(parentTitle);
    lcd.print("]");
  }

  // Scroll indicator top-right
  lcd.setCursor(13, 0);
  char scrollInd[4];
  snprintf(scrollInd, 4, "%d/%d", menuCursor + 1, currentMenu->itemCount);
  lcd.print(scrollInd);
}

void updateLCD() {
  // Read joystick
  int joy = readJoystick();
  switch (joy) {
    case 1: handleButtonUP(); break;     // Y up
    case 2: handleButtonDown(); break;   // Y down
    case 3: handleButtonBack(); break;   // X left = BACK
    case 4: handleButtonOK(); break;    // X right = OK
    case 5: handleButtonOK(); break;    // Push = OK
  }

  static unsigned long lastDraw = 0;
  if (millis() - lastDraw > 150) {
    lastDraw = millis();
    drawMenu();
  }
}