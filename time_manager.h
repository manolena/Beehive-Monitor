#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

enum TimeSource {
    TSRC_NONE = 0,
    TSRC_WIFI,
    TSRC_LTE
};

void   timeManager_init();
void   timeManager_update();
bool   timeManager_isTimeValid();
String timeManager_getDate();   // local, DD-MM-YYYY
String timeManager_getTime();   // local, HH:MM:SS
TimeSource timeManager_getSource();

#endif
