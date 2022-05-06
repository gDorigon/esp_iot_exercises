#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "esp_log.h"
#define PIN_SWITCH 15
#define PIN_LED 25

void app_main()
{

    gpio_pulldown_en(PIN_SWITCH);
    gpio_pullup_dis(PIN_SWITCH);
    int button = 0;
    while (true)
    {

        if (gpio_get_level(PIN_SWITCH) == 1)
        {
            vTaskDelay(pdMS_TO_TICKS(250));
            button += 1;
        }

        switch (button)
        {

        case 1:
            dac_output_enable(DAC_CHANNEL_1);
            dac_output_voltage(DAC_CHANNEL_1, 190);
            break;

        case 2:
            dac_output_enable(DAC_CHANNEL_1);
            dac_output_voltage(DAC_CHANNEL_1, 200);
            break;
        case 3:
            dac_output_enable(DAC_CHANNEL_1);
            dac_output_voltage(DAC_CHANNEL_1, 210);
            break;

        case 4:
            dac_output_enable(DAC_CHANNEL_1);
            dac_output_voltage(DAC_CHANNEL_1, 220);
            break;

        case 5:
            dac_output_enable(DAC_CHANNEL_1);
            dac_output_voltage(DAC_CHANNEL_1, 255);
            break;

        default:
            dac_output_enable(DAC_CHANNEL_1);
            dac_output_voltage(DAC_CHANNEL_1, 0);
            button = 0;
            break;
        }
    }
}
