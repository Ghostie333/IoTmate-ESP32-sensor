# IoTmate - firmware czujnika ESP32

Firmware czujnika DHT22 dla XIAO ESP32-C3. Urządzenie konfiguruje Wi-Fi przez portal, rejestruje się w backendzie IoTmate i publikuje odczyty temperatury oraz wilgotności przez MQTT.

## Wymagania

- XIAO ESP32-C3
- DHT22 zasilany z 3.3 V, linia danych na GPIO0, pull-up 4.7-10 kOhm
- Backend IoTmate oraz broker MQTT

## Portal konfiguracyjny

- SSID: `IoTmate-Sensor`
- Hasło: `iotmate-` + ostatnie 6 znaków adresu MAC (drukowane na porcie szeregowym)
- Przycisk resetu Wi-Fi: GPIO10 do GND

## Budowanie

Dev (TLS bez weryfikacji certyfikatu, HTTP backend 7070):

```sh
pio run
pio run --target upload
pio device monitor
```

Prod (weryfikacja CA, HTTPS backend 443):

```sh
pio run -e prod --target upload
```
