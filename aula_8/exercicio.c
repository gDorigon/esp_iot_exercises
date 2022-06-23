#include <stdio.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"
#include "sdkconfig.h"

#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "driver/dac.h"


#define SERVER_URI  "ws://172.16.107.19"
#define SERVER_PORT 8000
#define SERVER_PATH "/websocket"

static const char *TAG = "From_terminal";

const char* opCodeToString(char num) {
	switch(num){
		case 0x00: return "continuation";
		case 0x01: return "text";
		case 0x02: return "binary";
		case 0x08: return "connclose";
		case 0x09: return "ping";
		case 0x0A: return "pong";
	default: return "undefined";
	}
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        if (data->op_code == 0x08) {
            ESP_LOGW(TAG, "Received closed message with code=%d", 256*data->data_ptr[0] + data->data_ptr[1]);
        } else {
            if(data->data_len > 0) {
                printf("\x1b[2K\r");
                printf("%s\n",(char *)data->data_ptr);
                memset(data->data_ptr, 0, strlen(data->data_ptr));
            }

        }

        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}

static void websocket_app_start(esp_websocket_client_handle_t *client)
{
    esp_websocket_client_config_t websocket_cfg = {};

    websocket_cfg.uri = SERVER_URI;
    websocket_cfg.port = SERVER_PORT;
    websocket_cfg.path = SERVER_PATH;

    ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);

    *client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(*client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    esp_websocket_client_start(*client);

}

void ws_send(void* param)
{
    esp_websocket_client_handle_t* p_client = (esp_websocket_client_handle_t*) param;
    char c = 0;
    char *str = (char *)malloc(100);
    char *str2 = (char *)malloc(100);

    while(1) {
    // zerando conteudo de str
    memset(str, 0, 100);


    printf("\n\nDigite: ");

    // Enquanto não receber um "enter"
    while (c != '\n')
    {
        // Recebe dado pela serial
        c = getchar();

        // Caso a tecla seja "backspace"
        if (c == 0x08)
        {
            // Apaga o último caracter da string
            str[((strlen(str) - 1) > 0) ? strlen(str) - 1 : 0] = c;

            // Apaga o conteúdo da linha no terminal
            printf("\x1b[2K");

            // Imprime conteúdo de str no terminal
            printf("\rDigite: %.*s", strlen(str), str);
        
        }
        // Caso seja caracter válido
        else if ((c >= 0x20) && (c <= 0x7e))
        {
            // Insere caracter na ultima posição de str
            str[strlen(str)] = c;

            // Apaga o conteúdo da linha no terminal
            printf("\x1b[2K");

            // Imprime conteúdo de str no terminal
            printf("\rDigite: %.*s", strlen(str), str);
        }

    }
    sprintf(str2,"Dorigon: %s",str);
    esp_websocket_client_send_text(*p_client, str2, strlen(str2) , portMAX_DELAY); 
    c = 0;    
    printf("\x1b[2K\r");  
    printf("\rDorigon: %.*s", strlen(str), str);

    }
    esp_websocket_client_close(*p_client, portMAX_DELAY);
    ESP_LOGI(TAG, "Websocket Stopped");
    esp_websocket_client_destroy(*p_client);
    vTaskDelete(NULL);
    
}


void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    static esp_websocket_client_handle_t client;
    websocket_app_start(&client);
    xTaskCreate(ws_send, "ws_send", 2048, (void *)&client, 15, NULL);


}
