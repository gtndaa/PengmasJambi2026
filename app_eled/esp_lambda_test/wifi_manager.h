#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WifiManager
{
public:

    void begin();

    bool isConnected();

    void reconnect();

    IPAddress localIP();
};

#endif