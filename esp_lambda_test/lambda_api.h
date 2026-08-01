#ifndef LAMBDA_API_H
#define LAMBDA_API_H

class LambdaAPI
{
public:

    bool begin();

    bool getLatestSensor();

    bool postSensorData();

};

#endif