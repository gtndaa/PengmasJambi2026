#include "DHTTempSensor.h"
#include "headers.h"

void DHTTempSensor::begin(uint8_t pin) {
    dht = DHT(pin, DHT_TEMP_TYPE);
    dht.begin();
    began = true;
    delay(2000);
}

float DHTTempSensor::readHumidity() {
    if (!began) return NAN;
    float h = dht.readHumidity();
    if (isnan(h)) {
        LOG_WARN("[DHT-TEMP] Gagal baca humidity dari DHT (pin %d)", DHT_TEMP_PIN);
    } else {
        LOG_INFO("[DHT-TEMP] Humidity terbaca: %.1f%%", h);
    }
    return h;
}

float DHTTempSensor::readTemperature() {
    if (!began) return NAN;
    return dht.readTemperature();
}
