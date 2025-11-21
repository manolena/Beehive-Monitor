#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------
// Define modem type BEFORE including TinyGSM
// (taken from your original working project v20)
// ---------------------------------------------------------------------
#define TINY_GSM_MODEM_A7670        // <-- required for A7670 modules
#define TINY_GSM_RX_BUFFER   1024

#include <TinyGsmClient.h>

// Expose the global modem instance
TinyGsm& modem_get();

// ---------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------
bool modem_isNetworkRegistered();
int16_t modem_getRSSI();
String modem_getOperator();

void modemManager_init();
