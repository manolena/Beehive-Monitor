#ifndef PROVISIONING_UI_H
#define PROVISIONING_UI_H

#include <Arduino.h>

// Blocking UI: enter City & Country via 4-button interface.
// Call from a menu action (it will block until user saves or cancels).
void provisioning_ui_enterCityCountry();

#endif // PROVISIONING_UI_H