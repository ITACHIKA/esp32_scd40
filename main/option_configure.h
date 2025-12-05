#include "esp_common.h"
#include "freertos/queue.h"

extern QueueHandle_t uartInputQueue;
extern char *devName;
extern char *wifiSSID;
extern char *WifiPasswd;

extern char *mqttUri;
extern char *mqttUsername;
extern char *mqttPasswd;
extern char *mqttcliid;

extern char *devName;
extern char *devmac;

extern bool optionChange;

void optionConfigInit();