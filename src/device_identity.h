#ifndef DEVICE_IDENTITY_H
#define DEVICE_IDENTITY_H

#include <Arduino.h>

// Global variables
extern String DEVICE_ID;
extern String HARDWARE_SECRET;

void generateDeviceId();
void generateHardwareUniqueId();

#endif // DEVICE_IDENTITY_H