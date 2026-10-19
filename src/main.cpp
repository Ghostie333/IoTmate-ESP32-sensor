#include <Arduino.h>
#include <esp_system.h>

#include "device_identity.h"
#include "mqtt_manager.h"
#include "sensor_manager.h"
#include "wifi_manager.h"

CustomWiFiManager wifiMgr;
SensorManager sensorMgr;
MqttManager mqttMgr(sensorMgr);

void setup()
{
    Serial.begin(115200);
    delay(3000);
    Serial.println("\n=== IoTmate XIAO ESP32-C3 Sensor Firmware Starting ===");

    Serial.printf("[SYSTEM] Reset reason: %d\n", esp_reset_reason());

    generateDeviceId();
    generateHardwareUniqueId();

    Serial.print("Device ID (MAC): ");
    Serial.println(DEVICE_ID);

    sensorMgr.begin();
    wifiMgr.setupWiFi();
    mqttMgr.begin();
}

void loop()
{
    wifiMgr.loop();
    mqttMgr.loop();
}
