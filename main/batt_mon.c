#include "driver/gpio.h"
#include "esp_log.h"
#include "batt_mon.h"

static const char *TAG = "BATT";

batt_cfg battery_module_cfg;
adc_oneshot_unit_handle_t adc_oneshot_handle;
adc_cali_handle_t adc_cali_handle;
const uint8_t BATTERY_TABLE_SIZE=21;

static const uint16_t battery_voltage_mv[] = {
    4200, 4150, 4110, 4080, 4020,
    3980, 3950, 3910, 3870, 3850,
    3840, 3820, 3800, 3790, 3770,
    3750, 3730, 3710, 3690, 3610,
    3300
};

static const uint8_t battery_percent[] = {
    100, 95, 90, 85, 80,
    75, 70, 65, 60, 55,
    50, 45, 40, 35, 30,
    25, 20, 15, 10, 5,
    0
};

const char* batt_stat_str[]={
    "No Power",
    "Charger idle",
    "INVALID",
    "Charge done",
    "INVALID",
    "Charging",
    "INVALID",
    "LDO Mode"
};

static uint8_t battery_level_linear_interpo(uint16_t mv)
{
    for (int i = 0; i < BATTERY_TABLE_SIZE - 1; i++) {
        if (mv >= battery_voltage_mv[i + 1]) {
            uint16_t v1 = battery_voltage_mv[i];
            uint16_t v2 = battery_voltage_mv[i + 1];
            uint8_t  p1 = battery_percent[i];
            uint8_t  p2 = battery_percent[i + 1];

            return p2 + (mv - v2) * (p1 - p2) / (v1 - v2);
        }
    }
    return 0;
}

void batt_mon_init(batt_cfg batt_module_pins)
{
    battery_module_cfg=batt_module_pins;
    adc_oneshot_unit_init_cfg_t adc_oneshot_unit_cfg={
        .unit_id=batt_module_pins.adc_unit,
        .ulp_mode=ADC_ULP_MODE_DISABLE,
        .clk_src=ADC_RTC_CLK_SRC_DEFAULT
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_oneshot_unit_cfg,&adc_oneshot_handle));
    adc_oneshot_chan_cfg_t adc_oneshot_chan_cfg={
        .atten=ADC_ATTEN_DB_12,
        .bitwidth=ADC_BITWIDTH_DEFAULT
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_oneshot_handle,batt_module_pins.adc_channel,&adc_oneshot_chan_cfg));
    adc_cali_scheme_ver_t cali_scheme;
    adc_cali_check_scheme(&cali_scheme);
    #if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3
    {
        //curve fitting is only available in esp32s3 and c3
        adc_cali_curve_fitting_config_t cali_cfg={
        .atten=ADC_ATTEN_DB_12,
        .bitwidth=ADC_BITWIDTH_DEFAULT,
        .chan=batt_module_pins.adc_channel,
        .unit_id=batt_module_pins.adc_unit
        };
        ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg,&adc_cali_handle));
    }
    #endif
    #if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
    {
        //line fitting is only available in esp32 and s2
        adc_cali_line_fitting_config_t cali_cfg={
        .atten=ADC_ATTEN_DB_12,
        .bitwidth=ADC_BITWIDTH_DEFAULT,
        .unit_id=batt_module_pins.adc_unit
        };
        ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_cfg,&adc_cali_handle));
    }
    #endif
    gpio_config_t gpio_conf = {
        .pin_bit_mask = ((1ULL << batt_module_pins.pg_pin) | (1ULL << batt_module_pins.s1_pin) | (1ULL << batt_module_pins.s2_pin)),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&gpio_conf);
    ESP_LOGI(TAG,"BATT module init done.");
}

/*
    Return reading value in mV
*/
float get_batt_voltage()
{
    int adc_raw_value;
    int adc_raw_voltage;
    float adc_voltage;
    adc_oneshot_read(adc_oneshot_handle,battery_module_cfg.adc_channel,&adc_raw_value);
    adc_cali_raw_to_voltage(adc_cali_handle,adc_raw_value,&adc_raw_voltage);
    adc_voltage=(float)adc_raw_voltage*(10.+24.)/24.; // resistor voltage divider
    return adc_voltage;
}

/*
    Return battery level based on raw voltage, using linear interpolation
*/
uint8_t get_battery_level()
{
    uint16_t raw_voltage=get_batt_voltage();
    uint8_t batt_level=battery_level_linear_interpo(raw_voltage);
    return batt_level;
}

/*
    Return MCP73833 status code
*/
batt_status get_batt_status()
{
    uint8_t pg=gpio_get_level(battery_module_cfg.pg_pin);
    uint8_t s1=gpio_get_level(battery_module_cfg.s1_pin);
    uint8_t s2=gpio_get_level(battery_module_cfg.s2_pin);
    return ((!pg << 0)|(!s2 << 1)|(!s1 << 2)); // by datasheet
}