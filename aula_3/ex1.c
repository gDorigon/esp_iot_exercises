#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
xSemaphoreHandle mutexBus;
const char *TAG = "mutex.c";

#define PIN_SWITCH 15
#define PIN_LED 23

int buttonsend = 0;
bool state = 0;

void buttonstatus(void *param)
{
    while (true)
    {
        int level = gpio_get_level(PIN_SWITCH);

        if(level == 1){
            xSemaphoreTake(mutexBus, 1000);
            printf("button status = %d\n", buttonsend);
            buttonsend = 1;
            xSemaphoreGive(mutexBus);
        }
        else{
            xSemaphoreTake(mutexBus, 1000);
            buttonsend = 0;
            xSemaphoreGive(mutexBus);
        }
        
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void setfq(void *param){
    while(true){

        state = !state;

        if(buttonsend == 1){
            gpio_set_level(PIN_LED, state);
            vTaskDelay(pdMS_TO_TICKS(30));
        }
        else{
            gpio_set_level(PIN_LED, state);
            vTaskDelay(pdMS_TO_TICKS(1000));
    
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_SWITCH);
    gpio_set_direction(PIN_SWITCH, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pulldown_en(PIN_SWITCH);
    gpio_pullup_dis(PIN_SWITCH);

    mutexBus = xSemaphoreCreateMutex();

    xTaskCreate(&setfq, "frequency", 2048, NULL, 1, NULL);
    xTaskCreate(&buttonstatus, "pin state", 2048, NULL, 1, NULL);


}
