#ifndef SMS_HANDLER_H
#define SMS_HANDLER_H

#include <Arduino.h>

// Initialize SMS handler (call during setup after modemManager_init).
void sms_init();

// Call periodically from loop() to check for unread messages and process them.
void sms_loop();

#endif // SMS_HANDLER_H