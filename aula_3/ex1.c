#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#define PIN_SWITCH 15
#define PIN_LED 23
const char *TAG = "input.c";

xSemaphoreHandle mutexBus;
xSemaphoreHandle mutexBus2;

bool isOn = 0;
int level = 0;

void fast(void *params)
{
    while (true)
    {
        if (xSemaphoreTake(mutexBus2, 1000))
        {
            isOn = !isOn;
            gpio_set_level(PIN_LED, isOn);
            vTaskDelay(pdMS_TO_TICKS(10000));
            printf(" MODO FAST");
        }
    }
    vTaskDelete(NULL);
}

void low(void *params)
{
    while (true)
    {
        if (xSemaphoreTake(mutexBus, 1000))
        {
            isOn = !isOn;
            gpio_set_level(PIN_LED, isOn);
            vTaskDelay(pdMS_TO_TICKS(5000));
            printf(" MODO LOW");
        }
    }
    vTaskDelete(NULL);
}

void button(void *params)
{
    while (true)
    {
        level = gpio_get_level(PIN_SWITCH);

        if (level == 1)
        {
            printf("BOTAO 1");
             xSemaphoreGive(mutexBus);
        }
        else
        {
            printf("BOTAO 0");
            xSemaphoreGive(mutexBus2);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    printf("Switch and LED\n");
    ESP_LOGW(TAG, "Exercicio 1 rodando");
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_SWITCH);
    gpio_set_direction(PIN_SWITCH, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pulldown_en(PIN_SWITCH);
    gpio_pullup_dis(PIN_SWITCH);

    mutexBus = xSemaphoreCreateMutex();
    mutexBus2 = xSemaphoreCreateMutex();

    xTaskCreate(&fast, "100ms", 2048, NULL, 2, NULL);
//    xTaskCreate(&low, "500ms", 2048, NULL, 2, NULL);
    xTaskCreate(&button, "botão", 2048, NULL, 2, NULL);
}

