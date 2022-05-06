#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"

void app_main(){
    int measurement = 0; 
    while(true){
        measurement = hall_sensor_read();
        printf("measurement: %d\n", measurement);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
