/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"


// biblioteca DHT11
#include "dht.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
// -----------------


#include "esp_log.h"
#include "mqtt_client.h"

#include "freertos/event_groups.h"
#include "driver/gpio.h"

static const char *TAG = "MQTT_EXAMPLE";

QueueHandle_t rcv_queue = NULL;

typedef struct receive {
    char topic[100];
    char data[100];
} rcv_t;

// WiFi network info.
char ssid[] = "PADOLABS";
char wifiPassword[] = "P@d0l@bs";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
#define BROKER "mqtt://mqtt.mydevices.com"
#define USERNAME "95759790-e29c-11ec-a681-73c9540e1265"
#define PASSWORD "43b782d47d76e9bf21de0a639f018b2f12368876"
#define CLIENT_ID "ca80b190-e29c-11ec-8c44-371df593ba58"

// https://developers.mydevices.com/cayenne/docs/cayenne-mqtt-api/#cayenne-mqtt-api-supported-data-types
// https://developers.mydevices.com/cayenne/docs/cayenne-mqtt-api/#cayenne-mqtt-api-mqtt-messaging-topics-examples

#define MAIN_TOPIC  "v1/" USERNAME "/things/" CLIENT_ID
#define RSP_TOPIC MAIN_TOPIC "/response"

#define RSSI_CH "1"
#define MEM_CH "2"
#define BTN_CH  "3"

#define RSSI_TOPIC MAIN_TOPIC "/data/" RSSI_CH
#define RSSI_MSG_PREFIX "rel_hum,p="

#define MEM_TOPIC MAIN_TOPIC "/data/" MEM_CH
#define MEM_MSG_PREFIX "temp,c="

#define BTN_TOPIC_PUB MAIN_TOPIC "/data/" BTN_CH
#define BTN_TOPIC_SUB MAIN_TOPIC "/cmd/" BTN_CH

#define PIN_LED 23
#define WIFI_CONNECTED_BIT BIT0
#define MQTT_CONNECTED_BIT BIT1

EventGroupHandle_t events;
esp_mqtt_client_handle_t client;
TaskHandle_t rcv_handle, send_handle;

int temperatura=0;

static void log_error_if_nonzero(const char * message, int errorCode)
{
    if (errorCode != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, errorCode);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    rcv_t rcv;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(events, MQTT_CONNECTED_BIT);

            msg_id = esp_mqtt_client_subscribe(client, BTN_TOPIC_SUB, 0);
            ESP_LOGI(TAG, "msg_id=%d | sent subscribe successful", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupClearBits(events, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "msg_id=%d | MQTT_EVENT_SUBSCRIBED", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "msg_id=%d | MQTT_EVENT_UNSUBSCRIBED", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "msg_id=%d | MQTT_EVENT_PUBLISHED", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

            memcpy(rcv.topic, event->topic, (event->topic_len > sizeof(rcv.topic) -1 ? sizeof(rcv.topic)-1:event->topic_len));
            memcpy(rcv.data, event->data, (event->data_len > sizeof(rcv.data)-1? sizeof(rcv.data)-1: event->data_len));
            rcv.data[(event->data_len > sizeof(rcv.data)? sizeof(rcv.data): event->data_len)] = '\0';
            xQueueSend(rcv_queue, &rcv, 0);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = BROKER,
        .port = 1883,
        .username = USERNAME,
        .password = PASSWORD,
        .client_id = CLIENT_ID
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void rcv_task(void* param) 
{
    rcv_t received;
    char seq[20] = {0};
    char cmd[3] = {0};
    char message[30] = {0};

    while(1)
    {
        memset(seq, 0, sizeof(seq));
        memset(cmd, 0, sizeof(cmd));
        memset(message, 0, sizeof(message));
        if( xQueueReceive(rcv_queue, &received, portMAX_DELAY) )
        {
            ESP_LOGI(TAG, "received message %.*s", strlen(received.data),received.data);
            //https://stackoverflow.com/a/15006320/19124287
            char *pos = strchr(received.data, 44); //44 = ,
            if (pos != NULL)
            {
                memcpy(seq, received.data, pos-received.data);
                memcpy(cmd, pos + 1, 1);
        
                sprintf(message, "ok,%s",seq);
                esp_mqtt_client_publish(client, RSP_TOPIC, message, strlen(message), 1, 0);
                
                if(strcmp(cmd, "1") == 0) 
                {
                    gpio_set_level(PIN_LED, true);
                } 
                else if (strcmp(cmd, "0") == 0)
                {
                    gpio_set_level(PIN_LED, false);
                }
                memset(message, 0, sizeof(message));

                sprintf(message, "%d", gpio_get_level(PIN_LED));
                esp_mqtt_client_publish(client, BTN_TOPIC_PUB, message, strlen(message), 1, 0);
            }
            

        }
    }
}

void send_task(void *param)
{
    DHT11_init(GPIO_NUM_25);
    
    wifi_ap_record_t ap_info;
    char message[20] = {0};
    while(1)
    {        
        EventBits_t bits = xEventGroupWaitBits(events, MQTT_CONNECTED_BIT,
                    pdFALSE, pdFALSE, 0);
        if(bits & MQTT_CONNECTED_BIT) 
        {
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
            {
                sprintf(message, RSSI_MSG_PREFIX"%d", DHT11_read().humidity);
                int msg_id = esp_mqtt_client_publish(client, RSSI_TOPIC, message, strlen(message), 1, 0);
                ESP_LOGW(TAG, "msg_id=%d | sending %.*s", msg_id, strlen(message),message);
            }

            memset(message, 0, sizeof(message));
            sprintf(message, MEM_MSG_PREFIX"%d", DHT11_read().temperature);
            int msg_id = esp_mqtt_client_publish(client, MEM_TOPIC, message, strlen(message), 1, 0);

            ESP_LOGW(TAG, "msg_id=%d | sending %.*s", msg_id, strlen(message),message);

            
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{

    
    
    // temperatura = DHT11_read().temperature;

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    events = xEventGroupCreate();
    rcv_queue = xQueueCreate(10, sizeof(rcv_t));

    xTaskCreate(rcv_task, "rcv_task", 4096, NULL, 12, &rcv_handle);
    xTaskCreate(send_task, "send_task", 2048, NULL, 12, &send_handle);

    mqtt_app_start();
}
