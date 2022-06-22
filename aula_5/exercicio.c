/*  WiFi softAP Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "html_support.h"
#include "web_server.h"
#include "freertos/event_groups.h"
#include "esp_http_client.h"

/* The examples use WiFi configuration that you can set via project
   configuration menu.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_WIFI_SSID "Dorigas"
#define EXAMPLE_ESP_WIFI_PASS "dorigon1"
#define EXAMPLE_ESP_WIFI_CHANNEL 1
#define EXAMPLE_MAX_STA_CONN 4

#define WIFI_SSID "Cambalacho"
#define WIFI_PASS "mypassword"

#define WIFI_SSID "PADOLABS"
#define WIFI_PASS "P@d0l@bs"

static EventGroupHandle_t wifi_events;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define MAX_RETRY 10
static int retry_cnt = 0;
static void request_page(void *);
static esp_err_t handle_http_event(esp_http_client_event_t *);
static void handle_wifi_connection(void *, esp_event_base_t, int32_t, void *);

static const char *TAG = "wifi softAP";

esp_netif_t *my_ap = NULL;
QueueHandle_t xQueueHttp;

static void init_wifi(void)
{
    if (nvs_flash_init() != ESP_OK)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_events = xEventGroupCreate();
    esp_event_loop_create_default();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                               &handle_wifi_connection, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                               &handle_wifi_connection, NULL);

    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = WIFI_SSID,
                .password = WIFI_PASS,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg = {.capable = true, .required = false},
            },
    };
    esp_netif_init();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    EventBits_t bits =
        xEventGroupWaitBits(wifi_events, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                            pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        xTaskCreate(request_page, "http_req", 5 * configMINIMAL_STACK_SIZE,
                    NULL, 5, NULL);
    }
    else
    {
        ESP_LOGE(TAG, "failed");
    }
}

static void handle_wifi_connection(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (retry_cnt++ < MAX_RETRY)
        {
            esp_wifi_connect();
            ESP_LOGI(TAG, "wifi connect retry: %d", retry_cnt);
        }
        else
        {
            xEventGroupSetBits(wifi_events, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "ip: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
        retry_cnt = 0;
        xEventGroupSetBits(wifi_events, WIFI_CONNECTED_BIT);
    }
}

static void request_page(void *arg)
{
    esp_http_client_config_t config = {
        .url = "https://www.google.com/",
        .event_handler = handle_http_event,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (esp_http_client_perform(client) != ESP_OK)
    {
        ESP_LOGE(TAG, "http request failed");
    }
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event =
            (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGW(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
                 event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event =
            (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGE(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac),
                 event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    my_ap = esp_netif_create_default_wifi_ap();
    esp_netif_dhcpc_stop(my_ap);

    // esp_netif_ip_info_t ip_info;

    // IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    // IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    // IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    // esp_netif_set_ip_info(my_ap, &ip_info);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {.ssid = EXAMPLE_ESP_WIFI_SSID,
               .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
               .channel = EXAMPLE_ESP_WIFI_CHANNEL,
               .password = EXAMPLE_ESP_WIFI_PASS,
               .max_connection = EXAMPLE_MAX_STA_CONN,
               .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGE(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS,
             EXAMPLE_ESP_WIFI_CHANNEL);
}

void app_main(void)
{
    esp_netif_ip_info_t ip;

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize SPIFFS
    ESP_LOGI(TAG, "Initializing SPIFFS");
    if (SPIFFS_Mount("/html", "storage", 6) != ESP_OK)
    {
        ESP_LOGE(TAG, "SPIFFS mount failed");
        while (1)
        {
            vTaskDelay(1);
        }
    }

    xQueueHttp = xQueueCreate(10, sizeof(post_messages_t));
    configASSERT(xQueueHttp);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();

    start_webserver();

    esp_netif_get_ip_info(my_ap, &ip);
    ESP_LOGI(TAG, "ip = " IPSTR, IP2STR(&ip.ip));
    post_messages_t postMessage;
    while (1)
    {
        // Waiting for submit
        if (xQueueReceive(xQueueHttp, &postMessage, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGW(TAG, "message:%.*s", postMessage.lenght,
                     postMessage.message);
            if (strcmp(postMessage.message, "ssid=Dorigas&password=dorigon1") ==
                0)
            {
                ESP_LOGI(TAG, "SENHA CORRETA");
                //init_wifi();
            }
            else
            {
                ESP_LOGE(TAG, "SENHA INCORRETA");
            }
