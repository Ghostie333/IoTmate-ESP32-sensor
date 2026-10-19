#include "mqtt_manager.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>

#include "certificates.h"
#include "config.h"
#include "device_identity.h"
#include "sensor_manager.h"

MqttManager::MqttManager(SensorManager &sensorManager)
    : _mqttClient(_wifiClient), _sensorManager(sensorManager) {}

void MqttManager::begin()
{
    _topicStatus = "devices/" + DEVICE_ID + "/status";
    _topicState = "devices/" + DEVICE_ID + "/state";

    _mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    _mqttClient.setBufferSize(512);
    _mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC);
    _mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_SEC);

#ifndef MQTT_DEV_TLS
    _wifiClient.setCACert(ROOT_CA);
#else
    _wifiClient.setInsecure();
#endif
}

void MqttManager::loop()
{
    const unsigned long now = millis();

    if (_state == MqttState::Connected)
    {
        if (!_mqttClient.connected())
        {
            _mqttClient.loop();
            _state = MqttState::Disconnected;
            return;
        }

        _mqttClient.loop();

        if (now - _lastPublish >= SENSOR_INTERVAL_MS)
        {
            publishReadings();
        }
        return;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        _retryDelayMs = MQTT_RETRY_DELAY_MS;
        return;
    }

    if (now - _lastConnectAttempt >= _retryDelayMs)
    {
        _lastConnectAttempt = now;

        if (reconnect())
        {
            _state = MqttState::Connected;
            _retryDelayMs = MQTT_RETRY_DELAY_MS;
            return;
        }

        const unsigned long nextDelay = _retryDelayMs * 2;
        _retryDelayMs = nextDelay > MQTT_RETRY_MAX_DELAY_MS ? MQTT_RETRY_MAX_DELAY_MS : nextDelay;
        _retryDelayMs += random(0, 1000);
    }
}

bool MqttManager::reconnect()
{
    if (WiFi.status() != WL_CONNECTED)
        return false;

    const MqttCredentials creds = fetchCredentials(DEVICE_ID, _forceCredentialRefresh);
    _forceCredentialRefresh = false;

    if (creds.user.isEmpty() || creds.pass.isEmpty())
    {
        Serial.println("[MQTT] No credentials to connect.");
        return false;
    }

    static const char lwtPayload[] = "{\"online\":false}";

    const bool connected = _mqttClient.connect(
        creds.user.c_str(),
        creds.user.c_str(),
        creds.pass.c_str(),
        _topicStatus.c_str(),
        1,
        true,
        lwtPayload);

    if (connected)
    {
        Serial.println("[MQTT] Connected successfully!");
        _mqttClient.publish(_topicStatus.c_str(), "{\"online\":true}", true);
        publishReadings();
        return true;
    }

    const int state = _mqttClient.state();
    Serial.printf("[MQTT] Connection error, rc=%d\n", state);

    if (state == 5)
    {
        Serial.println("[MQTT] Credentials rejected, refreshing registration on next attempt.");
        _forceCredentialRefresh = true;
    }

    return false;
}

void MqttManager::publishReadings()
{
    _lastPublish = millis();

    if (!_sensorManager.read())
    {
        Serial.println("[SENSOR] Read failed, skipping publish.");
        return;
    }

    const String json = _sensorManager.getReadingsAsJson();
    if (json.isEmpty())
    {
        Serial.println("[SENSOR] No valid readings to publish.");
        return;
    }

    if (_mqttClient.publish(_topicState.c_str(), json.c_str(), false))
    {
        Serial.printf("[MQTT] Published readings: %s\n", json.c_str());
    }
    else
    {
        Serial.println("[MQTT] Failed to publish readings.");
    }
}

MqttCredentials MqttManager::fetchCredentials(const String &deviceId, bool forceRefresh)
{
    Preferences prefs;
    prefs.begin("mqtt_config", false);

    const String savedUser = prefs.getString("user", "");
    const String savedPass = prefs.getString("pass", "");

    if (!forceRefresh && savedUser.length() > 0 && savedPass.length() > 0)
    {
        Serial.println("[PROVISION] Loaded MQTT credentials from NVS.");
        prefs.end();
        return {savedUser, savedPass};
    }

    HTTPClient http;
    const String backendAddress = String(BACKEND_SCHEME) + "://" + String(BACKEND_SERVER) + ":" + String(BACKEND_PORT) + "/api/devices/register";

    Serial.print("[HTTP] Connecting to backend: ");
    Serial.println(backendAddress);

#if defined(BACKEND_TLS)
    http.begin(_wifiClient, backendAddress);
#else
    http.begin(_backendClient, backendAddress);
#endif
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");

    JsonDocument request;
    request["deviceId"] = deviceId;
    request["deviceSecret"] = HARDWARE_SECRET;
    request["deviceType"] = static_cast<int>(DEVICE_TYPE);

    String body;
    serializeJson(request, body);

    const int httpCode = http.POST(body);

    if (httpCode == HTTP_CODE_OK)
    {
        const String response = http.getString();
        JsonDocument doc;

        if (!deserializeJson(doc, response))
        {
            const String user = doc["deviceId"].as<String>();
            const String pass = doc["secretToken"].as<String>();

            if (user.length() > 0 && pass.length() > 0)
            {
                prefs.putString("user", user);
                prefs.putString("pass", pass);
                http.end();
                prefs.end();
                Serial.println("[PROVISION] Saved new MQTT credentials to NVS.");
                return {user, pass};
            }
        }

        Serial.println("[PROVISION] Registration response was invalid.");
    }
    else if (httpCode == 401)
    {
        Serial.println("[PROVISION] Registration rejected: invalid device secret.");
    }
    else
    {
        Serial.printf("[HTTP] Request failed, HTTP code: %d\n", httpCode);
    }

    http.end();
    prefs.end();
    return {savedUser, savedPass};
}
