#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>
#include "config.h"

class SensorManager
{
public:
    SensorManager();

    void begin();
    bool read();
    String getReadingsAsJson() const;

private:
    DHT _dht;
    float _temperature = NAN;
    float _humidity = NAN;
    bool _hasReading = false;
};

#endif // SENSOR_MANAGER_H
