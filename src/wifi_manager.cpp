#include "wifi_manager.h"
#include <WiFiManager.h>
#include <WiFi.h>
#include <Arduino.h>
#include <esp_system.h>
#include "device_identity.h"

void CustomWiFiManager::setupWiFi()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    checkResetButtonOnBoot();

    _wm.setCaptivePortalEnable(true);
    _wm.setConnectTimeout(10);
    _wm.setSaveConnectTimeout(10);
    _wm.setConfigPortalTimeout(0);

    String savedSSID = _wm.getWiFiSSID();
    if (savedSSID.length() == 0)
    {
        Serial.println("[WiFi] No saved credentials found, opening config portal...");
        startConfigPortalLoop();
        return;
    }

    Serial.println("[WiFi] Saved credentials found for: " + savedSSID);

    if (connectToSavedWiFi(savedSSID, _wm.getWiFiPass()))
    {
        Serial.print("[WiFi] Connected, IP: ");
        Serial.println(WiFi.localIP());
        _lastReconnectAttempt = millis();
        return;
    }

    Serial.println("[WiFi] Could not connect to saved WiFi after multiple attempts.");
    Serial.println("[WiFi] Keeping saved credentials and opening config portal...");
    WiFi.mode(WIFI_AP_STA);

    startConfigPortalLoop();
}

void CustomWiFiManager::loop()
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    const unsigned long now = millis();
    if (now - _lastReconnectAttempt < WIFI_RECONNECT_INTERVAL_MS)
        return;

    _lastReconnectAttempt = now;
    Serial.println("[WiFi] Connection lost, reconnecting...");
    WiFi.reconnect();
}

bool CustomWiFiManager::connectToSavedWiFi(const String &ssid, const String &pass)
{
    for (uint8_t attempt = 1; attempt <= WIFI_CONNECT_ATTEMPTS; attempt++)
    {
        Serial.printf("[WiFi] Connecting to %s (attempt %u/%u)...\n",
                      ssid.c_str(), attempt, WIFI_CONNECT_ATTEMPTS);

        if (tryConnectOnce(ssid, pass, WIFI_CONNECT_TIMEOUT_MS))
        {
            return true;
        }

        WiFi.disconnect(false, false);
        delay(200);
    }

    return false;
}

bool CustomWiFiManager::tryConnectOnce(const String &ssid, const String &pass, unsigned long timeoutMs)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs)
    {
        delay(500);
    }

    return WiFi.status() == WL_CONNECTED;
}

void CustomWiFiManager::startConfigPortalLoop()
{
    const String portalPassword = String(WIFI_PORTAL_PASSWORD_PREFIX) + DEVICE_ID.substring(6);

    while (true)
    {
        WiFi.softAPdisconnect(true);
        Serial.printf("[WiFi] Config portal '%s' password: %s\n", WIFI_PORTAL_SSID, portalPassword.c_str());

        bool connected = _wm.startConfigPortal(WIFI_PORTAL_SSID, portalPassword.c_str());

        if (connected)
        {
            Serial.print("[WiFi] Connected, IP: ");
            Serial.println(WiFi.localIP());
            _lastReconnectAttempt = millis();
            return;
        }

        Serial.println("[WiFi] Config portal closed without connecting. Restarting portal...");
        delay(500);
        WiFi.mode(WIFI_AP_STA);
    }
}

void CustomWiFiManager::checkResetButtonOnBoot()
{
    pinMode(RESET_PIN, INPUT_PULLUP);

    delay(100);

    bool held = (digitalRead(RESET_PIN) == LOW);
    if (held)
    {
        delay(80);
        held = (digitalRead(RESET_PIN) == LOW);
    }

    if (held)
    {
        Serial.println("\n[SYSTEM] RESET_BUTTON LONG PRESS detected");
        Serial.println("[SYSTEM] Cleaning Wi-Fi settings...");

        _wm.resetSettings();

        while (digitalRead(RESET_PIN) == LOW)
        {
            delay(50);
        }

        Serial.println("[SYSTEM] Settings cleared. Launching Captive Portal...");
    }
}
