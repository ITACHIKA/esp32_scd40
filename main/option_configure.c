#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "option_configure.h"
#include "uart_service.h"
#include "nvs_service.h"
#include <string.h>

QueueHandle_t uartInputQueue;
static bool selectedOption = false;
static bool optionChange = false;

static char *target;

char *wifiSSID;
char *WifiPasswd;

char *mqttUri;
char *mqttUsername;
char *mqttPasswd;
char *mqttcliid;

void inputOptionHandler(void *pvParameters)
{
    char *ptrToBuffer;
    for (;;)
    {
        if (xQueueReceive(uartInputQueue, &ptrToBuffer, portMAX_DELAY))
        {
            if (ptrToBuffer != NULL)
            {
                if (selectedOption == false)
                {
                    if (strcmp(ptrToBuffer, "ssid") == 0)
                    {
                        esp_rom_printf("Enter wifi ssid:");
                        target = wifiSSID;
                        selectedOption = true;
                    }
                    else if (strcmp(ptrToBuffer, "wifipass") == 0)
                    {
                        esp_rom_printf("Enter wifi password:");
                        target = WifiPasswd;
                        selectedOption = true;
                    }
                    else if (strcmp(ptrToBuffer, "uri") == 0)
                    {
                        esp_rom_printf("Enter MQTT server uri:");
                        target = mqttUri;
                        selectedOption = true;
                    }
                    else if (strcmp(ptrToBuffer, "mqttusr") == 0)
                    {
                        esp_rom_printf("Enter MQTT username:");
                        target = mqttUsername;
                        selectedOption = true;
                    }
                    else if (strcmp(ptrToBuffer, "mqttpwd") == 0)
                    {
                        esp_rom_printf("Enter MQTT username:");
                        target = mqttPasswd;
                        selectedOption = true;
                    }
                    else if (strcmp(ptrToBuffer, "mqttcliid") == 0)
                    {
                        esp_rom_printf("Enter MQTT client id:");
                        target = mqttcliid;
                        selectedOption = true;
                    }
                    else if (strcmp(ptrToBuffer, "view") == 0)
                    {
                        esp_rom_printf("Current config:\r\n");
                        esp_rom_printf("Wifi SSID:%s\r\n", wifiSSID);
                        esp_rom_printf("Wifi passwd:%s\r\n", WifiPasswd);
                        esp_rom_printf("mqtt server uri:%s\r\n", mqttUri);
                        esp_rom_printf("mqtt username:%s\r\n", mqttUsername);
                        esp_rom_printf("mqtt passwd:%s\r\n", mqttPasswd);
                        esp_rom_printf("mqtt cli id:%s\r\n", mqttcliid);
                        if (optionChange == true)
                        {
                            esp_rom_printf("Option(s) changed and not saved.\r\n");
                        }
                    }
                    else if (strcmp(ptrToBuffer, "save") == 0)
                    {
                        if (optionChange)
                        {
                            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("wifissid", wifiSSID));
                            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("wifipass", WifiPasswd));
                            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("mqtturi", mqttUri));
                            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("mqttusr", mqttUsername));
                            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("mqttpwd", mqttPasswd));
                            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("mqttcliid", mqttcliid));
                            esp_restart();
                        }
                    }
                    else
                    {
                        esp_rom_printf("Available command: ssid wifipass uri mqttusr mqttpwd mqttcliid view save\r\n");
                    }
                }
                else
                {
                    strcpy(target, ptrToBuffer);
                    selectedOption = false;
                    optionChange = true;
                }
            }
        }
    }
    free(ptrToBuffer);
    free(wifiSSID);
    free(WifiPasswd);
    free(mqttUsername);
    free(mqttPasswd);
    free(mqttUri);
}
void optionConfigInit()
{
    uartInputQueue = xQueueCreate(10, sizeof(char *));
    wifiSSID = calloc(32, 1);
    WifiPasswd = calloc(64, 1);
    mqttUsername = calloc(64, 1);
    mqttPasswd = calloc(128, 1);
    mqttUri = calloc(128, 1);
    mqttcliid = calloc(23, 1);
    nvs_utils_get_str("wifissid", wifiSSID, 32);
    nvs_utils_get_str("wifipass", WifiPasswd, 64);
    nvs_utils_get_str("mqtturi", mqttUri, 128);
    nvs_utils_get_str("mqttusr", mqttUsername, 64);
    nvs_utils_get_str("mqttpwd", mqttPasswd, 128);
    nvs_utils_get_str("mqttcliid", mqttcliid, 23);
    xTaskCreate(inputOptionHandler, "inputOptionHandler", 2048, NULL, 4, NULL);
}
