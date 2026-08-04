#include "mqtt_manager.h"

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "config.h"
#include "secrets.h"



WiFiClientSecure secureClient;

PubSubClient mqttClient(secureClient);



void mqttCallback(
    char* topic,
    byte* payload,
    unsigned int length
)
{

    Serial.println();
    Serial.println("MQTT Message Received");

    Serial.print("Topic : ");
    Serial.println(topic);


    String message;


    for(int i=0;i<length;i++)
    {
        message += (char)payload[i];
    }


    Serial.print("Payload : ");
    Serial.println(message);


}




void MqttManager::begin()
{

    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("Initializing MQTT...");
    Serial.println("--------------------------------");


    secureClient.setInsecure();
    /*
       sementara untuk testing.
       Nanti production pakai CA certificate HiveMQ
    */


    mqttClient.setServer(
        MQTT_HOST,
        MQTT_PORT
    );


    mqttClient.setCallback(
        mqttCallback
    );


}



void MqttManager::reconnect()
{

    Serial.println("Connecting MQTT...");


    while(!mqttClient.connected())
    {

        if(
            mqttClient.connect(
                DEVICE_NAME,
                MQTT_USERNAME,
                MQTT_PASSWORD
            )
        )
        {

            Serial.println(
                "MQTT Connected"
            );


            // Subscribe

            mqttClient.subscribe(
                TOPIC_CONFIG
            );


            mqttClient.subscribe(
                TOPIC_COMMAND
            );


            publishStatus();


        }

        else
        {

            Serial.print(
                "MQTT Failed. State="
            );

            Serial.println(
                mqttClient.state()
            );


            delay(5000);

        }

    }

}




void MqttManager::loop()
{

    if(!mqttClient.connected())
    {
        reconnect();
    }


    mqttClient.loop();

}




bool MqttManager::isConnected()
{
    return mqttClient.connected();
}




void MqttManager::publishStatus()
{

    String payload;


    payload =
    "{"
    "\"device\":\""
    DEVICE_NAME
    "\","
    "\"status\":\"online\""
    "}";


    mqttClient.publish(
        TOPIC_STATUS,
        payload.c_str()
    );


    Serial.println(
        "Status Published"
    );

}