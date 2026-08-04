#ifndef LAMBDA_API_H
#define LAMBDA_API_H

#include "device_config.h"

class LambdaAPI
{
public:

    bool begin();

    bool getLatestSensor();

    bool postSensorData();
    
    bool getDeviceConfig(DeviceConfig &config);

};

#endif