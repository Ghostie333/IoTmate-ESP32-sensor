#include "sensor_manager.h"

#include <ArduinoJson.h>

SensorManager::SensorManager()
    : _dht(DHT_PIN, DHT_TYPE) {}

void SensorManager::begin()
{
    _hasReading = false;
    _dht.begin();
}

bool SensorManager::read()
{
    const float temperature = _dht.readTemperature();
    const float humidity = _dht.readHumidity();

    if (isnan(temperature) || isnan(humidity))
        return false;

    _temperature = temperature;
    _humidity = humidity;
    _hasReading = true;
    return true;
}

String SensorManager::getReadingsAsJson() const
{
    if (!_hasReading)
        return String();

    JsonDocument doc;
    doc["temperature"] = _temperature;
    doc["humidity"] = _humidity;

    String json;
    serializeJson(doc, json);
    return json;
}
