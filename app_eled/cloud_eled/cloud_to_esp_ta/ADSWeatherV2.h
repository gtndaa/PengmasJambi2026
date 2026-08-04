#include "Arduino.h"

#ifndef ADSWeatherV2_h
#define ADSWeatherV2_h

class ADSWeatherV2
{
public:
    ADSWeatherV2(int rainPin, int windDirPin, int windSpdPin);

    float getRain();
    float getWindDirection();
    float getWindSpeed();
    float getWindGust();

    void update();

    static void IRAM_ATTR countRain();
    static void IRAM_ATTR countAnemometer();

private:
    int _rainPin;
    int _windDirPin;
    int _windSpdPin;

    float _rain;
    float _windDir;
    float _windSpd;
    float _windSpdMax;

    unsigned long _nextCalc;
    unsigned long _timer;

    unsigned int _vaneSample[15];   // 50 samples from the sensor for consensus averaging
    unsigned int _vaneSampleIdx;
    unsigned int _windDirBin[10];
    unsigned int _gust[30];         // Array of 30 wind speed values to calculate maximum gust speed.
    unsigned int _gustIdx;

    float _readRainAmount();
    float _readWindDir();
    float _readWindSpd();
    void _setBin(unsigned int windVane);
};

#endif