#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFiManager.h>
#include "config.h"

class CustomWiFiManager
{
public:
    CustomWiFiManager() = default;

    void setupWiFi();
    void loop();

private:
    WiFiManager _wm;
    unsigned long _lastReconnectAttempt = 0;

    void checkResetButtonOnBoot();
    bool connectToSavedWiFi(const String &ssid, const String &pass);
    bool tryConnectOnce(const String &ssid, const String &pass, unsigned long timeoutMs);
    void startConfigPortalLoop();
};

#endif
