#pragma once
#include <Arduino.h>
#include "config.h"

enum Button {
    BTN_NONE = 0,
    BTN_UP_PRESSED,
    BTN_DOWN_PRESSED,
    BTN_SELECT_PRESSED,
    BTN_BACK_PRESSED
};

enum Language {
    LANG_EN = 0,
    LANG_GR = 1
};

extern Language currentLanguage;

Button getButton();
void uiInit();
void uiClear();
void uiPrint(uint8_t col, uint8_t row, const char *msg);

// Greek character functions
void initGreekChars();
void lcdPrintGreek(const char *utf8str, uint8_t col, uint8_t row);
