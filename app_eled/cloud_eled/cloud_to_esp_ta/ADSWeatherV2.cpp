#include "Arduino.h"
#include "ADSWeatherV2.h"

#define DEBOUNCE_TIME 15
#define CALC_INTERVAL 1000

volatile int _anemometerCounter;
volatile int _rainCounter;
volatile unsigned long last_micros_rg;
volatile unsigned long last_micros_an;

// Initialization routine. This function sets up the pins on the ESP32 and initializes variables.
ADSWeatherV2::ADSWeatherV2(int rainPin, int windDirPin, int windSpdPin)
{
    // Initialization routine
    _anemometerCounter = 0;
    _rainCounter = 0;
    _gustIdx = 0;
    _vaneSampleIdx = 0;

    _rainPin = rainPin;
    _windDirPin = windDirPin;
    _windSpdPin = windSpdPin;

    pinMode(_rainPin, INPUT_PULLUP);
    pinMode(_windSpdPin, INPUT_PULLUP);
    pinMode(_windDirPin, INPUT);
}

// The update function updates the values of all of the sensor variables. This should be run as frequently as possible
// in the main loop for maximum precision.
void ADSWeatherV2::update()
{
    _timer = millis();
    _vaneSample[_vaneSampleIdx] = analogRead(_windDirPin);
    _vaneSampleIdx++;
    if(_vaneSampleIdx >= 15)
    {
        _vaneSampleIdx = 0;
    }
    if(_timer > _nextCalc)
    {
        _nextCalc = _timer + CALC_INTERVAL;
        // UPDATE ALL VALUES
        _rain += _readRainAmount();
        _windSpd = _readWindSpd();
        _windDir = _readWindDir();
    }
}

// Returns the amount of rain since the last time the getRain function was called.
float ADSWeatherV2::getRain()
{
    return _rain;
}

// Returns the direction of the wind in degrees.
float ADSWeatherV2::getWindDirection()
{
    return _windDir;
}

// Returns the wind speed.
float ADSWeatherV2::getWindSpeed()
{
    return _windSpd;
}

// Returns the maximum wind gust speed.
float ADSWeatherV2::getWindGust()
{
    return _windSpdMax;
}

// Updates the rain amount internal state.
float ADSWeatherV2::_readRainAmount()
{
    float rain = 0;
    rain = 11 * _rainCounter;
    _rainCounter = 0;
    return rain;
}

// Updates the wind direction internal state.
float ADSWeatherV2::_readWindDir()
{
    unsigned int maximum, sum;
    unsigned char i, j, max_i;
    // Clear wind vane averaging bins
    for (i = 0; i < 16; i++)
    {
        _windDirBin[i] = 0;
    }
    // Read all samples into bins
    for (i = 0; i < 15; i++)
    {
        _setBin(_vaneSample[i]);
    }
    // Find the bin with the highest sum
    maximum = 0;
    for (i = 0; i < 16; i++)
    {
        if (_windDirBin[i] > maximum)
        {
            maximum = _windDirBin[i];
            max_i = i;
        }
    }
    // Map bin to the corresponding wind direction angle
    switch (max_i)
    {
        case 0: return 0;     // N
        case 1: return 22.5;  // NNE
        case 2: return 45;    // NE
        case 3: return 67.5;  // ENE
        case 4: return 90;    // E
        case 5: return 112.5; // ESE
        case 6: return 135;   // SE
        case 7: return 157.5; // SSE
        case 8: return 180;   // S
        case 9: return 202.5; // SSW
        case 10: return 225;  // SW
        case 11: return 247.5; // WSW
        case 12: return 270;  // W
        case 13: return 292.5; // WNW
        case 14: return 315;  // NW
        case 15: return 337.5; // NNW
        default: return -1;   // Error case
    }
}

// Returns the wind speed since the last calcInterval.
float ADSWeatherV2::_readWindSpd()
{
    unsigned char i;
    float spd = 24000;
    spd *= _anemometerCounter;
    spd /= 10000;
    _anemometerCounter = 0;
    if(_gustIdx > 29)
    {
        _gustIdx = 0;
    }
    _gust[_gustIdx] = (int) spd;
    for (i = 0; i < 30; i++)
    {
        if (_gust[i] > _windSpdMax) _windSpdMax = _gust[i];
    }
    return (float) spd;
}

// Internal function for calculating the wind direction using specific direction angles
void ADSWeatherV2::_setBin(unsigned int windVane)
{
    // Read wind directions into bins
    unsigned char bin;
    if (windVane > 3800) bin = 12;      // W (292.5 - 337.5)
    else if (windVane > 3550) bin = 14; // NW (337.5 - 360, 0 - 22.5)
    else if (windVane > 3230) bin = 13; // WNW (247.5 - 292.5)
    else if (windVane > 2960) bin = 0;  // N (22.5 - 67.5)
    else if (windVane > 2680) bin = 15; // NNW (0 - 22.5, 337.5 - 360)
    else if (windVane > 2400) bin = 10; // SW (202.5 - 247.5)
    else if (windVane > 2260) bin = 11; // WSW (225 - 270)
    else if (windVane > 1680) bin = 2;  // NE (67.5 - 112.5)
    else if (windVane > 1460) bin = 1;  // NNE (45 - 90)
    else if (windVane > 980) bin = 8;   // S (135 - 180)
    else if (windVane > 830) bin = 9;   // SSW (157.5 - 202.5)
    else if (windVane > 580) bin = 6;   // SE (112.5 - 157.5)
    else if (windVane > 330) bin = 7;   // SSE (90 - 135)
    else if (windVane > 230) bin = 4;   // E (67.5 - 112.5)
    else if (windVane > 190) bin = 3;   // ESE (45 - 90)
    else bin = 5;                       // S (22.5 - 67.5)
    _windDirBin[bin]++;
}

// ISR for rain gauge.
void IRAM_ATTR ADSWeatherV2::countRain()
{
    if ((long) (micros() - last_micros_rg) >= DEBOUNCE_TIME * 1000)
    {
        _rainCounter++;
        last_micros_rg = micros();
    }
}

// ISR for anemometer.
void IRAM_ATTR ADSWeatherV2::countAnemometer()
{
    if ((long) (micros() - last_micros_an) >= DEBOUNCE_TIME * 1000)
    {
        _anemometerCounter++;
        last_micros_an = micros();
    }
}