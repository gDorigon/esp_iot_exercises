#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"

void magnetico(void *arg){

    int mag=0;

    while(true) {

    mag = hall_sensor_read();
    printf("measurement: %d\n", mag);
    vTaskDelay(pdMS_TO_TICKS(100));

    }

    vTaskDelay(NULL);
}

void app_main(){
    int measurement = 0; 


    while(true){

    xTaskCreatePinnedToCore(&magnetico, "mag reading", 2048, NULL, 2, NULL, 1);
    vTaskDelay(300);

    }
}

//by Dorigas
