#include <stdio.h>
#include "mqtt_client.h"
#include "option_configure.h"

esp_mqtt_client_handle_t mqttclient;

void mqtt_init()
{
    esp_mqtt_client_config_t config={
        .broker.address.uri=mqttUri,
        //.broker.address.uri="mqtt://192.168.1.100:1883",
    };
    if(strcmp(mqttUsername,"")!=0)
    {
        config.credentials.username=mqttUsername;
    }
    if(strcmp(mqttPasswd,"")!=0)
    {
        config.credentials.authentication.password=mqttPasswd;
    }
    if(strcmp(mqttcliid,"")!=0)
    {
        config.credentials.client_id=mqttcliid;
    }

    mqttclient = esp_mqtt_client_init(&config);
    if(!mqttclient)
    {
        esp_rom_printf("mqtt client init fail.");
        return;
    }
    esp_mqtt_client_start(mqttclient);
}

void mqtt_publish(const char* topic,const char* msg)
{
    esp_mqtt_client_publish(mqttclient,topic,msg,0,1,0);
}