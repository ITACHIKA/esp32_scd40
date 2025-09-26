#include <driver/uart.h>
#include <driver/gpio.h>
#include <string.h>
#include <option_configure.h>
#include "esp_log.h"

// #define USB_CDC

#define UART_NUM UART_NUM_0 // UART0
#define UART_TX_PIN 43
#define UART_RX_PIN 44
#define BUF_SIZE 1024
#define RD_BUF_SIZE 8

static QueueHandle_t uartEventQueue;

void uartEventQueueHandler(void *pvParameters)
{
    char *stringBuffer = malloc(64);
    size_t buffLen = 0;
    uart_event_t event;
    uint8_t dtmp;
    for (;;)
    {
        if (xQueueReceive(uartEventQueue, (void *)&event, portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
                size_t len = uart_read_bytes(UART_NUM, &dtmp, event.size, portMAX_DELAY);
                if (len > 0)
                {
                    uart_write_bytes(UART_NUM, &dtmp, sizeof(dtmp));
                    if (dtmp == '\r' || dtmp == '\n')
                    {
                        esp_rom_printf("\r\n");
                        stringBuffer[buffLen] = '\0';
                        char *stringCopy = malloc(64);
                        strcpy(stringCopy, stringBuffer);
                        xQueueSend(uartInputQueue, &stringCopy, portMAX_DELAY);
                        buffLen = 0;
                    }
                    else if (dtmp == 0x08 || dtmp == 0x7F)
                    {
                        if (buffLen > 0)
                        {
                            buffLen--;
                            stringBuffer[buffLen] = '\0';
                        }
                        continue;
                    }
                    else
                    {
                        stringBuffer[buffLen++] = dtmp;
                    }
                }
                break;
            case UART_FIFO_OVF:
                uart_flush_input(UART_NUM);
                xQueueReset(uartEventQueue);
                break;
            default:
                break;
            }
        }
    }
    free(stringBuffer);
}

void uart_init()
{
#ifndef USB_CDC
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM, &uart_config);

    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 16, &uartEventQueue, 0);
    xTaskCreate(uartEventQueueHandler, "uartEventQueueHandler", 4096, NULL, 4, NULL);
#else

#endif
}

void uartSend(const char *str)
{
    if (str != NULL)
    {
        uart_write_bytes(UART_NUM, str, strlen(str));
    }
}