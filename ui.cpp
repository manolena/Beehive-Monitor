#include "config.h"
#include "ui.h"                // bring Language, Button, prototypes into scope
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// ui.h should declare Language, Button, currentLanguage, and prototypes used below.
// We do NOT change any Greek LCD mapping or behavior.

extern LiquidCrystal_I2C lcd;

// --------------------------------------------------
// BUTTON HANDLING
// --------------------------------------------------
Button getButton()
{
    static uint32_t lastTime = 0;
    static bool upLast   = true;
    static bool downLast = true;
    static bool selLast  = true;
    static bool backLast = true;

    uint32_t now = millis();
    if (now - lastTime < 120)   // strong debounce
        return BTN_NONE;
    lastTime = now;

    bool upNow   = digitalRead(BTN_UP);
    bool downNow = digitalRead(BTN_DOWN);
    bool selNow  = digitalRead(BTN_SELECT);
    bool backNow = digitalRead(BTN_BACK);

    if (upLast && !upNow)       { upLast = upNow;   return BTN_UP_PRESSED; }
    if (downLast && !downNow)   { downLast = downNow; return BTN_DOWN_PRESSED; }
    if (selLast && !selNow)     { selLast = selNow; return BTN_SELECT_PRESSED; }
    if (backLast && !backNow)   { backLast = backNow; return BTN_BACK_PRESSED; }

    upLast   = upNow;
    downLast = downNow;
    selLast  = selNow;
    backLast = backNow;

    return BTN_NONE;
}

// --------------------------------------------------
// UI INIT
// --------------------------------------------------
void uiInit() {
    pinMode(BTN_UP,     INPUT_PULLUP);
    pinMode(BTN_DOWN,   INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);
    pinMode(BTN_BACK,   INPUT_PULLUP);

    lcd.init();
    lcd.backlight();
    lcd.clear();

    initGreekChars();
}

void uiClear() {
    lcd.clear();
}

// --------------------------------------------------
// PRINT WRAPPER (EN/GR)
// --------------------------------------------------
void uiPrint(uint8_t col, uint8_t row, const char *msg) {
    if (currentLanguage == LANG_GR)
        lcdPrintGreek(msg, col, row);
    else {
        lcd.setCursor(col, row);
        lcd.print(msg);
    }
}

// --------------------------------------------------
// GREEK CHAR SYSTEM
// --------------------------------------------------
void initGreekChars() {
  byte Gamma[8]  = { B11111, B10000, B10000, B10000, B10000, B10000, B10000, B00000 };
  byte Delta[8]  = { B00100, B01010, B10001, B10001, B10001, B10001, B11111, B00000 };
  byte Lambda[8] = { B00100, B01010, B10001, B10001, B10001, B10001, B10001, B00000 };
  byte Xi[8]     = { B11111, B00000, B00000, B01110, B00000, B00000, B11111, B00000 };
  byte Pi[8]     = { B11111, B10001, B10001, B10001, B10001, B10001, B10001, B00000 };
  byte Phi[8]    = { B01110, B10101, B10101, B10101, B01110, B00100, B00100, B00000 };
  byte Psi[8]    = { B10101, B10101, B10101, B01110, B00100, B00100, B00100, B00000 };
  byte Omega[8]  = { B01110, B10001, B10001, B10001, B01110, B00000, B11111, B00000 };

  lcd.createChar(0, Gamma);
  lcd.createChar(1, Delta);
  lcd.createChar(2, Lambda);
  lcd.createChar(3, Xi);
  lcd.createChar(4, Pi);
  lcd.createChar(5, Phi);
  lcd.createChar(6, Psi);
  lcd.createChar(7, Omega);
}

void lcdPrintGreek(const char *utf8str, uint8_t col, uint8_t row) {
    lcd.setCursor(col, row);
    const char *p = utf8str;

    while (*p) {
        if ((uint8_t)*p == 0xCE || (uint8_t)*p == 0xCF) {
            uint8_t first = (uint8_t)*p;
            p++;
            uint8_t second = (uint8_t)*p;

            if (first == 0xCE) {
                switch (second) {
                    case 0x91: lcd.write('A'); break;      // Α
                    case 0x92: lcd.write('B'); break;      // Β
                    case 0x93: lcd.write((uint8_t)0); break; // Γ
                    case 0x94: lcd.write((uint8_t)1); break; // Δ
                    case 0x95: lcd.write('E'); break;      // Ε
                    case 0x96: lcd.write('Z'); break;      // Ζ
                    case 0x97: lcd.write('H'); break;      // Η
                    case 0x98: lcd.write(242); break;      // Θ
                    case 0x99: lcd.write('I'); break;      // Ι
                    case 0x9A: lcd.write('K'); break;      // Κ
                    case 0x9B: lcd.write((uint8_t)2); break; // Λ
                    case 0x9C: lcd.write('M'); break;      // Μ
                    case 0x9D: lcd.write('N'); break;      // Ν
                    case 0x9E: lcd.write((uint8_t)3); break; // Ξ
                    case 0x9F: lcd.write('O'); break;      // Ο
                    case 0xA0: lcd.write((uint8_t)4); break; // Π
                    case 0xA1: lcd.write('P'); break;      // Ρ
                    case 0xA3: lcd.write(246); break;      // Σ
                    case 0xA4: lcd.write('T'); break;      // Τ
                    case 0xA5: lcd.write('Y'); break;      // Υ
                    case 0xA6: lcd.write((uint8_t)5); break; // Φ
                    case 0xA7: lcd.write('X'); break;      // Χ
                    case 0xA8: lcd.write((uint8_t)6); break; // Ψ
                    case 0xA9: lcd.write((uint8_t)7); break; // Ω
                    default: lcd.write('?'); break;
                }
            }
        } else {
            lcd.write(*p);
        }
        p++;
    }
}