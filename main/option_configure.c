#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "option_configure.h"
#include "uart_service.h"
#include "nvs_service.h"
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include <string.h>
#include "driver/i2c.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define SCD40_I2C_ADDR 0x62

QueueHandle_t uartInputQueue;
static bool selectedOption = false;
static bool optionChange = false;

static char *target;

static struct
{
    struct arg_str *help_cmd;
    struct arg_int *verbose_level;
    struct arg_end *end;
} help_args;

char *wifiSSID;
char *WifiPasswd;

char *mqttUri;
char *mqttUsername;
char *mqttPasswd;
char *mqttcliid;

void read_nvs()
{
    nvs_utils_get_str("wifissid", wifiSSID, 32);
    nvs_utils_get_str("wifipass", WifiPasswd, 64);
    nvs_utils_get_str("mqtturi", mqttUri, 128);
    nvs_utils_get_str("mqttusr", mqttUsername, 64);
    nvs_utils_get_str("mqttpwd", mqttPasswd, 128);
    nvs_utils_get_str("mqttcliid", mqttcliid, 23);
}

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
                    else if (strcmp(ptrToBuffer, "discard") == 0)
                    {
                        read_nvs();
                    }
                    else if (strcmp(ptrToBuffer, "calibr") == 0)
                    {
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

static esp_err_t scd_write_command(uint16_t cmd)
{
    uint8_t send_seq[2];
    send_seq[0] = cmd >> 8;
    send_seq[1] = cmd;
    return i2c_master_write_to_device(I2C_MASTER_NUM, SCD40_I2C_ADDR, send_seq, 2, pdMS_TO_TICKS(500));
}

static esp_err_t scd_calibr(uint16_t target_ppm)
{
    uint8_t buf[4];
    buf[0] = 0x36;
    buf[1] = 0x2F;

    buf[2] = (target_ppm >> 8) & 0xFF;
    buf[3] = target_ppm & 0xFF;
    return i2c_master_write_to_device(I2C_MASTER_NUM, SCD40_I2C_ADDR, buf, 4, pdMS_TO_TICKS(500));
}

static struct
{
    struct arg_str *cfg_str;
    struct arg_str *cfg_val;
    struct arg_end *end;
} set_args;

int set_command(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&set_args);

    if (nerrors != 0)
    {
        arg_print_errors(stderr, set_args.end, argv[0]);
        return 0;
    }
    const char *config_str = set_args.cfg_str->sval[0];
    const char *config_val = set_args.cfg_val->sval[0];
    if (strcmp(config_str, "wifissid") == 0)
    {
        if (strcmp(config_val, wifiSSID) != 0)
        {
            strcpy(wifiSSID, config_val);
            optionChange = true;
        }
    }
    else if (strcmp(config_str, "wifipass") == 0)
    {
        if (strcmp(config_val, WifiPasswd) != 0)
        {
            strcpy(WifiPasswd, config_val);
            optionChange = true;
        }
    }
    else if (strcmp(config_str, "uri") == 0)
    {
        if (strcmp(config_val, mqttUri) != 0)
        {
            strcpy(mqttUri, config_val);
            optionChange = true;
        }
    }
    else if (strcmp(config_str, "mqttusr") == 0)
    {
        if (strcmp(config_val, mqttUsername) != 0)
        {
            strcpy(mqttUsername, config_val);
            optionChange = true;
        }
    }
    else if (strcmp(config_str, "mqttpwd") == 0)
    {
        if (strcmp(config_val, mqttPasswd) != 0)
        {
            strcpy(mqttPasswd, config_val);
            optionChange = true;
        }
    }
    else if (strcmp(config_str, "mqttcliid") == 0)
    {
        if (strcmp(config_val, mqttcliid) != 0)
        {
            strcpy(mqttcliid, config_val);
            optionChange = true;
        }
    }
    else
    {
        esp_rom_printf("Unknown param\r\n");
    }
    return 0;
}
esp_err_t esp_console_register_set_command(void)
{
    set_args.cfg_str = arg_str1(NULL, NULL, "<configuration to set>", "param to set");
    set_args.cfg_val = arg_str1(NULL, NULL, "<configuration value>", "value of param");
    set_args.end = arg_end(2);

    esp_console_cmd_t command = {
        .command = "set",
        .help = "Set a variety of MCU configs.",
        .func = &set_command,
        .argtable = &set_args};
    return esp_console_cmd_register(&command);
}

int view_command()
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
    return 0;
}

esp_err_t esp_console_register_view_command(void)
{
    esp_console_cmd_t command = {
        .command = "view",
        .help = "view current MCU configs.",
        .func = &view_command,
        .argtable = NULL};
    return esp_console_cmd_register(&command);
}

int save_command()
{
    if (optionChange == true)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("wifissid", wifiSSID));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("wifipass", WifiPasswd));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("mqtturi", mqttUri));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("mqttusr", mqttUsername));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("mqttpwd", mqttPasswd));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_utils_set_str("mqttcliid", mqttcliid));
        esp_rom_printf("The system will now reboot...\r\n");
        esp_restart();
    }
    else
    {
        esp_rom_printf("No change need to be saved.\r\n");
    }
    return 0;
}

esp_err_t esp_console_register_save_command(void)
{
    esp_console_cmd_t command = {
        .command = "save",
        .help = "save current MCU configs.",
        .func = &save_command,
        .argtable = NULL};
    return esp_console_cmd_register(&command);
}

int unsave_command()
{
    read_nvs();
    esp_rom_printf("Changes cleared.\r\n");
    return 0;
}

esp_err_t esp_console_register_unsave_command(void)
{
    esp_console_cmd_t command = {
        .command = "unsave",
        .help = "unsave previously made changes.",
        .func = &unsave_command,
        .argtable = NULL};
    return esp_console_cmd_register(&command);
}

static struct
{
    struct arg_int *co2_ppm;
    struct arg_end *end;
} calibr_args;

int calibr_command(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&calibr_args);

    if (nerrors != 0)
    {
        arg_print_errors(stderr, calibr_args.end, argv[0]);
        return 0;
    }
    esp_rom_printf("Make sure the system have been running in the environment for 3 minutes prior to running the force calibration...\r\n");
    esp_rom_printf("Calibration will now start.\r\n");
    scd_write_command(0x3f86);
    vTaskDelay(pdMS_TO_TICKS(500));
    int target_ppm = calibr_args.co2_ppm->ival[0];
    scd_calibr(target_ppm);
    vTaskDelay(pdMS_TO_TICKS(400));
    return 0;
}

esp_err_t esp_console_register_calibr_command(void)
{
    calibr_args.co2_ppm = arg_int1(NULL, NULL, "<co2 ppm>", "co2 concentration(ppm) in current environment");
    calibr_args.end = arg_end(2);

    esp_console_cmd_t command = {
        .command = "calibr",
        .help = "Calibrate SCD40 co2 sensor in environment of known PPM.",
        .func = &calibr_command,
        .argtable = &calibr_args};
    return esp_console_cmd_register(&command);
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
    read_nvs();
    esp_console_register_set_command();
    esp_console_register_view_command();
    esp_console_register_save_command();
    esp_console_register_unsave_command();
    
    // xTaskCreate(inputOptionHandler, "inputOptionHandler", 2048, NULL, 4, NULL);
}
