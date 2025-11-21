#ifndef KEY_SERVER_H
#define KEY_SERVER_H

#include <Arduino.h>

// Initialize server (no-op until WiFi connected). Call once in setup or leave out and call keyServer_loop() from loop().
// You must call keyServer_loop() regularly (from your main loop).
void keyServer_init();   // starts the server if WiFi connected (safe to call repeatedly)
void keyServer_loop();   // handle incoming HTTP requests; also auto-starts server when WiFi connects
void keyServer_stop();   // stop server

#endif // KEY_SERVER_H