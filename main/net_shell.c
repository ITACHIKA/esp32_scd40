#include "net_shell.h"
#include "esp_log.h"
#include "esp_console.h"
#include "option_configure.h"
#include "esp_timer.h"
#include "network_service.h"

#define WEBSHELL_RETRY_COUNT 10

static const char *TAG = "HTTPSERVER";
char *ws_cmd_result_return_buf;

static char *get_cfg()
{
    uint16_t buf_size = 512;
    char *cfg_str = calloc(1, buf_size);
    assert(cfg_str); // make sure pointer is not null
    snprintf(cfg_str + strlen(cfg_str), buf_size - strlen(cfg_str), "Current config:\r\n");
    snprintf(cfg_str + strlen(cfg_str), buf_size - strlen(cfg_str), "Device name:%s\r\n", devName);
    snprintf(cfg_str + strlen(cfg_str), buf_size - strlen(cfg_str), "Wifi SSID:%s\r\n", wifiSSID);
    snprintf(cfg_str + strlen(cfg_str), buf_size - strlen(cfg_str), "Wifi passwd:%s\r\n", WifiPasswd);
    snprintf(cfg_str + strlen(cfg_str), buf_size - strlen(cfg_str), "mqtt server uri:%s\r\n", mqttUri);
    snprintf(cfg_str + strlen(cfg_str), buf_size - strlen(cfg_str), "mqtt username:%s\r\n", mqttUsername);
    snprintf(cfg_str + strlen(cfg_str), buf_size - strlen(cfg_str), "mqtt passwd:%s\r\n", mqttPasswd);
    snprintf(cfg_str + strlen(cfg_str), buf_size - strlen(cfg_str), "mqtt cli id:%s\r\n", mqttcliid);
    if (optionChange == true)
    {
        snprintf(cfg_str + strlen(cfg_str), buf_size - strlen(cfg_str), "Option(s) changed and not saved.\r\n");
    }
    return cfg_str;
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len)
    {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
        char *msg = malloc(32);
        if (strcmp((const char *)ws_pkt.payload, "view") != 0)
        {
            uint8_t len = strlen((const char *)ws_pkt.payload) + 1;
            char *cmd = malloc(len);
            cmd = strncpy(cmd, (const char *)ws_pkt.payload, len);
            int *ret_code = malloc(sizeof(int));
            esp_err_t ret = esp_console_run(cmd, ret_code);
            if (*ret_code != 0 || ret != ESP_OK)
            {
                strcpy(msg, "Command execute error.");
            }
            else
            {
                strcpy(msg, "Command execute succeed.");
            }
            free(cmd);
            free(ret_code);
        }
        else
        {
            free(msg);
            msg = get_cfg();
        }
        // httpd_ws_send_data()
        httpd_ws_frame_t ret_pkt = {
            .type = HTTPD_WS_TYPE_TEXT,
            .payload = (uint8_t *)msg,
            .len = strlen(msg),
            .final = true,
        };
        httpd_ws_send_frame(req, &ret_pkt);
        free(msg);
    }
    return ESP_OK;
}

static const httpd_uri_t ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = ws_handler,
    .user_ctx = NULL,
    .is_websocket = true};

static esp_err_t discover_handler(httpd_req_t *req)
{
    char* resp = malloc(128);
    int64_t uptime = esp_timer_get_time();
    snprintf(resp,128,"{\"device\":\"%s\",\"name\":\"%s\",\"uptime\":\"%lld\"}",CONFIG_IDF_TARGET,devName,uptime);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

static const httpd_uri_t discover = {
    .uri = "/esp_discover",
    .method = HTTP_GET,
    .handler = discover_handler,
    .user_ctx = NULL,
    .is_websocket = true};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    uint8_t retry_count=0;
    // Start the httpd server
    while(!networkReady)
    {
        esp_rom_printf("Waiting for network ready to start webshell\r\n");
        vTaskDelay(pdMS_TO_TICKS(200));
        retry_count++;
        if(retry_count==WEBSHELL_RETRY_COUNT)
        {
            esp_rom_printf("Webshell start fail since network is not ready.\r\n");
            return NULL;
        }
    }
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Registering the ws handler
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ws);
        httpd_register_uri_handler(server, &discover);
        ws_cmd_result_return_buf = malloc(1024 * sizeof(char));
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}