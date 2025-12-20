#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

/*
    The adc_channel is not pin number! Refer to datasheet of chip for channel and corresponding GPIO
*/
typedef struct{
    adc_unit_t adc_unit;
    adc_channel_t adc_channel;
    uint8_t s1_pin;
    uint8_t s2_pin;
    uint8_t pg_pin;
} batt_cfg;

typedef enum{
    BATT_NOCHG,
    BATT_IDLE,
    BATT_INVALID1,
    BATT_CHG_DONE,
    BATT_INVALID2,
    BATT_CHG,
    BATT_INVALID3,
    BATT_LDO
} batt_status;

extern const char* batt_stat_str[];

void batt_mon_init(batt_cfg batt_module_pins);
float get_batt_level();
batt_status get_batt_status();