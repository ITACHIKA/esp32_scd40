#include "esp_wifi.h"
#include "nvs_service.h"
#include "option_configure.h"
#include "string.h"
#include "esp_event.h"
#include "esp_log.h"

#define WIFI_RECONN_INTV_MS 50

#define WIFI_RECONN_MAX_RETRIES 10

const char *TAG = "WF Handler";
bool networkReady = false;
static uint8_t retry_count=0;

static void wifiEventHandler(void *handlerargs, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA started, connecting...");
            // esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Connected to AP");
            networkReady = true;
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
        {  
            wifi_event_sta_disconnected_t *disconn = (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGW(TAG, "Disconnected from SSID:%s, reason:%d", disconn->ssid, disconn->reason);
            networkReady = false;
            retry_count++;
            if(retry_count<WIFI_RECONN_MAX_RETRIES)
            {
                esp_wifi_connect();
            }
            else
            {
                esp_rom_printf("WIFI connect retry max count reached, please check configuration!\r\n");
            }
            break;
        }
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void networkInit()
{
    if (strcmp(wifiSSID, "") == 0)
    {
        esp_rom_printf("Wifi config invalid. Please reset.");
    }
    else
    {
        esp_rom_printf("wifissid:%s\n", wifiSSID);
        //esp_rom_printf("wifipass:%s\n", WifiPasswd);
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
        esp_netif_set_hostname(sta_netif, devName);
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        esp_wifi_set_mode(WIFI_MODE_STA);

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifiEventHandler,
            NULL,
            NULL));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifiEventHandler,
            NULL,
            NULL));

        wifi_config_t wifi_cfg = {0};
        wifi_cfg.sta.threshold.authmode=WIFI_AUTH_OPEN;
        strncpy((char *)wifi_cfg.sta.ssid, wifiSSID, sizeof(wifi_cfg.sta.ssid));
        wifi_cfg.sta.ssid[sizeof(wifi_cfg.sta.ssid) - 1] = '\0';
        strncpy((char *)wifi_cfg.sta.password, WifiPasswd, sizeof(wifi_cfg.sta.password));
        wifi_cfg.sta.password[sizeof(wifi_cfg.sta.password) - 1] = '\0'; //to avoid ovf
        esp_wifi_set_ps(WIFI_PS_NONE);
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
        ESP_ERROR_CHECK(esp_wifi_start());
        
        ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(44)); //https://esp32.com/viewtopic.php?f=2&t=41899#p137764, guess for YD-ESP32-S3 or similar clones only
        
        esp_rom_printf("wifi connect: %d\r\n", esp_wifi_connect());
    }
}