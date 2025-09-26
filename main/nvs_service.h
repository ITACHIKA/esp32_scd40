#pragma once
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"

esp_err_t nvs_utils_init(void);
esp_err_t nvs_utils_set_str(const char *key, const char *value);
esp_err_t nvs_utils_get_str(const char *key, char *out, size_t max_len);