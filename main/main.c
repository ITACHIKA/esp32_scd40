#include "uart_service.h"
#include "option_configure.h"
#include "nvs_service.h"
#include "network_service.h"
#include "mqtt_service.h"
#include "driver/i2c.h"
#include "net_shell.h"
#include "esp_common.h"

static const char* TAG = "main";

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA 4
#define I2C_MASTER_SCL 5

#define I2C_MASTER_FREQ_HZ 100000

#define MAX_RETRIES_BEFORE_REBOOT 5

#define SCD40_I2C_ADDR 0x62

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF

TimerHandle_t scdReadTimer;

TaskHandle_t scd_read_task_handle;

bool fault_flag = false;
static uint8_t fail_cnt=0;

static char* mqtt_co2_topic;
static char* mqtt_atemp_topic;
static char* mqtt_rh_topic;

uint8_t sensirion_common_generate_crc(const uint8_t *data, uint16_t count)
{
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;
    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte)
    {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

static esp_err_t scd_write_command(uint16_t cmd)
{
    uint8_t send_seq[2];
    send_seq[0] = cmd >> 8;
    send_seq[1] = cmd;
    return i2c_master_write_to_device(I2C_MASTER_NUM, SCD40_I2C_ADDR, send_seq, 2, pdMS_TO_TICKS(500));
}

void scd_read_callback()
{
    xTaskNotifyGive(scd_read_task_handle);
}

void scd_read_data(void *pvParameters)
{
    for (;;)
    {
        uint8_t readBuffer[9] = {0};
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(fail_cnt==MAX_RETRIES_BEFORE_REBOOT)
        {
            esp_restart();
        }
        if(fault_flag)
        {
            if(scd_write_command(0x21b1)==ESP_OK)
            {
                ESP_LOGE(TAG,"SCD40 reinit OK");
                fault_flag=false;
                continue;
            }
            else
            {
                ESP_LOGE(TAG,"SCD40 reinit fail");
                fail_cnt++;
            }
        }
        if (!fault_flag)
        {
            if (scd_write_command(0xec05) != ESP_OK)
            {
                ESP_LOGE(TAG,"Write error,skip");
                fault_flag = true;
                fail_cnt++;
                continue;
            }
            vTaskDelay(pdMS_TO_TICKS(1)); // according to ds
            if (i2c_master_read_from_device(I2C_MASTER_NUM, SCD40_I2C_ADDR, readBuffer, 9, pdMS_TO_TICKS(1000)) != ESP_OK)
            {
                ESP_LOGE(TAG,"Read error,skip");
                fault_flag = true;
                fail_cnt++;
                continue;
            }
            bool co2_crc_res = (sensirion_common_generate_crc(readBuffer, 2) == readBuffer[2]);
            bool temp_crc_res = (sensirion_common_generate_crc(&readBuffer[3], 2) == readBuffer[5]);
            bool rh_crc_res = (sensirion_common_generate_crc(&readBuffer[6], 2) == readBuffer[8]);
            uint16_t co2_ppm = ((uint16_t)readBuffer[0] << 8 | readBuffer[1]);
            uint16_t amb_temp_raw = ((uint16_t)readBuffer[3] << 8 | readBuffer[4]);
            uint16_t rel_humi_raw = ((uint16_t)readBuffer[6] << 8 | readBuffer[7]);
            float amb_temp = -45.0f + 175.0f * ((float)amb_temp_raw / 65535.0f);
            float rel_humi = 100.0f * ((float)rel_humi_raw / 65535.0f);
            char result[32];
            if (co2_crc_res)
            {
                snprintf(result, sizeof(result), "{\"co2\":%u}", co2_ppm);
                mqtt_publish(mqtt_co2_topic, result);
            }
            if (temp_crc_res)
            {
                snprintf(result, sizeof(result), "{\"atemp\":%.2f}", amb_temp);
                mqtt_publish(mqtt_atemp_topic, result);
            }
            if (rh_crc_res)
            {
                snprintf(result, sizeof(result), "{\"rh\":%.2f}", rel_humi);
                mqtt_publish(mqtt_rh_topic, result);
            }
        }
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_utils_init());
    optionConfigInit();
    uart_init();
    networkInit();
    esp_log_level_set("wifi", ESP_LOG_WARN);
    mqtt_init();
    start_webserver();
    mqtt_co2_topic=calloc(1,128);
    mqtt_rh_topic=calloc(1,128);
    mqtt_atemp_topic=calloc(1,128);
    snprintf(mqtt_co2_topic,128,"sensor/%s/co2",devName);
    snprintf(mqtt_rh_topic,128,"sensor/%s/rh",devName);
    snprintf(mqtt_atemp_topic,128,"sensor/%s/atemp",devName);
    // put user code here
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ};
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    scdReadTimer = xTimerCreate("scdReadTimer", pdMS_TO_TICKS(5050), pdTRUE, NULL, scd_read_callback);
    vTaskDelay(pdMS_TO_TICKS(1000)); // wait for SCD40 ready
    xTaskCreate(scd_read_data, "SCD read Task", 4096, NULL, 5, &scd_read_task_handle);

    scd_write_command(0x21b1); // start periodic measurement
    xTimerStart(scdReadTimer, portMAX_DELAY);
}
