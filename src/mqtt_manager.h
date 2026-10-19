#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"

class SensorManager;

struct MqttCredentials
{
    String user;
    String pass;
};

class MqttManager
{
public:
    explicit MqttManager(SensorManager &sensorManager);

    void begin();
    void loop();

private:
    enum class MqttState : uint8_t
    {
        Disconnected,
        Connected
    };

    WiFiClientSecure _wifiClient;
    WiFiClient _backendClient;
    PubSubClient _mqttClient;
    SensorManager &_sensorManager;

    String _topicStatus;
    String _topicState;

    MqttState _state = MqttState::Disconnected;
    unsigned long _lastPublish = 0;
    unsigned long _lastConnectAttempt = 0;
    unsigned long _retryDelayMs = MQTT_RETRY_DELAY_MS;
    bool _forceCredentialRefresh = false;

    bool reconnect();
    void publishReadings();
    MqttCredentials fetchCredentials(const String &deviceId, bool forceRefresh);
};

#endif
