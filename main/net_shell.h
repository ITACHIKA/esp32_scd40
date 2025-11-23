#pragma once
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_eth.h"

httpd_handle_t start_webserver(void);