#include "device_identity.h"
#include <WiFi.h>
#include <esp_efuse.h>
#include <Preferences.h>

String DEVICE_ID = "";
String HARDWARE_SECRET = "";

void generateDeviceId()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);

    char idBuffer[13];
    snprintf(idBuffer, sizeof(idBuffer), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    DEVICE_ID = String(idBuffer);
}

void generateHardwareUniqueId()
{
    // eFuse BLOCK3 first
    uint8_t efuseKey[32] = {0};
    esp_err_t err = esp_efuse_read_block(EFUSE_BLK3, efuseKey, 0, 256);
    bool isEfuseValid = (err == ESP_OK);
    if (isEfuseValid)
    {
        bool allZero = true, allFF = true;
        for (int i = 0; i < 32; i++)
        {
            if (efuseKey[i] != 0x00)
                allZero = false;
            if (efuseKey[i] != 0xFF)
                allFF = false;
        }
        if (allZero || allFF)
            isEfuseValid = false;
    }

    if (isEfuseValid)
    {
        char hexSecret[65];
        for (int i = 0; i < 32; i++)
            snprintf(&hexSecret[i * 2], 3, "%02x", efuseKey[i]);
        hexSecret[64] = '\0';
        HARDWARE_SECRET = String(hexSecret);
        Serial.println("[SECURITY] Secret from eFuse (BLOCK3).");
        return;
    }

    // If no secret in eFuse - generate and save in NVS
    Preferences prefs;
    prefs.begin("device_sec", false);

    String saved = prefs.getString("secret", "");
    if (saved.length() == 64)
    {
        HARDWARE_SECRET = saved;
        Serial.println("[SECURITY] Secret from NVS (persistent).");
        prefs.end();
        return;
    }

    uint8_t rnd[32];
    for (int i = 0; i < 32; i++)
        rnd[i] = (uint8_t)(esp_random() & 0xFF);

    char hexSecret[65];
    for (int i = 0; i < 32; i++)
        snprintf(&hexSecret[i * 2], 3, "%02x", rnd[i]);
    hexSecret[64] = '\0';

    HARDWARE_SECRET = String(hexSecret);
    prefs.putString("secret", HARDWARE_SECRET);
    prefs.end();
    Serial.println("[SECURITY] Generated new random secret, saved to NVS.");
}