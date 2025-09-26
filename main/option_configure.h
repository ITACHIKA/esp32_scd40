#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t uartInputQueue;
extern char *wifiSSID;
extern char *WifiPasswd;

extern char *mqttUri;
extern char *mqttUsername;
extern char *mqttPasswd;
extern char *mqttcliid;

void optionConfigInit();