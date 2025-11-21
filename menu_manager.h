#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <Arduino.h>
#include "text_strings.h"
#include "calibration.h"

struct MenuItem {
    TextId   text;
    void   (*action)(void);
    MenuItem* next;
    MenuItem* prev;
    MenuItem* parent;
    MenuItem* child;
};

void menuInit();
void menuDraw();
void menuUpdate();

#endif
