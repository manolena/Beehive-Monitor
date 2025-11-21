#include "modem_manager.h"
#include <HardwareSerial.h>

// ---------------------------------------------------------
// Physical serial port for A7670 (your original config)
// ---------------------------------------------------------
HardwareSerial SerialAT(2);  // UART2 on ESP32

static TinyGsm* _modem = nullptr;

// ---------------------------------------------------------
// Accessor for global modem instance
// ---------------------------------------------------------
TinyGsm& modem_get() {
    return *_modem;
}

// ---------------------------------------------------------
// Initialization
// ---------------------------------------------------------
void modemManager_init()
{
    SerialAT.begin(115200, SERIAL_8N1, 26, 27);   // your pins in v20
    delay(300);

    static TinyGsm modemInstance(SerialAT);
    _modem = &modemInstance;

    modemInstance.restart();
    delay(500);

    modemInstance.sendAT("+CFUN=1");
    modemInstance.waitResponse(1000);
}

// ---------------------------------------------------------
// Network registration
// ---------------------------------------------------------
bool modem_isNetworkRegistered()
{
    int stat = modem_get().getRegistrationStatus();

    // 1 = registered (home)
    // 5 = registered (roaming)
    return (stat == 1 || stat == 5);
}

// ---------------------------------------------------------
// Signal quality
// ---------------------------------------------------------
int16_t modem_getRSSI()
{
    return modem_get().getSignalQuality();
}

// ---------------------------------------------------------
// Operator
// ---------------------------------------------------------
String modem_getOperator()
{
    return modem_get().getOperator();
}
