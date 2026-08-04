#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H


class MqttManager
{

public:

    void begin();

    void loop();

    bool isConnected();

    void publishStatus();


private:

    void reconnect();

};


#endif