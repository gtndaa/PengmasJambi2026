#ifndef DHT_TEMP_SENSOR_H
#define DHT_TEMP_SENSOR_H

#include <DHT.h>

#ifndef DHT_TEMP_TYPE
#define DHT_TEMP_TYPE DHT22
#endif

class DHTTempSensor {
public:
    void begin(uint8_t pin);
    float readHumidity();
    float readTemperature();
private:
    DHT dht{0, DHT_TEMP_TYPE};
    bool began = false;
};

#endif
