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
/* The examples use WiFi configuration that you can set via project
   configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_WIFI_SSID "ESP-DORIGAS"
#define EXAMPLE_ESP_WIFI_PASS "dorigon123"
#define EXAMPLE_ESP_WIFI_CHANNEL 1
#define EXAMPLE_MAX_STA_CONN 4
#define DEFAULT_SCAN_LIST_SIZE 20
#define GPIO_BUTTON 21

static const char *TAG = "wifi softAP";

esp_netif_t *my_ap = NULL;
QueueHandle_t xQueueHttp;


static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event =
            (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
                 event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event =
            (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac),
                 event->aid);
    }
}


void wifi_init_softap(uint8_t flag)
{

    if (flag == 1){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    my_ap = esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    esp_netif_dhcpc_stop(my_ap);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    }
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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(
        WIFI_IF_AP, &wifi_config)); // apos credenciais mudar para sta;
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS,
             EXAMPLE_ESP_WIFI_CHANNEL);
}

void init_wifi(uint8_t *SSID, char *PASSWORD)
{
    // configuração de sta, para se conectar a rede wifi
    wifi_config_t wifi_config = {0};

    memcpy(wifi_config.sta.ssid, (char *)SSID, strlen((char *)SSID));
    memcpy(wifi_config.sta.password, PASSWORD, strlen(PASSWORD));

    printf("Conectado ao WIFI: %s \n", (char *)SSID);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    esp_wifi_connect();

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

    ESP_LOGI(TAG, "ESP_WIFI_MODE_APSTA");
    wifi_init_softap(1);

    start_webserver();

    esp_netif_get_ip_info(my_ap, &ip);
    ESP_LOGI(TAG, "ip = " IPSTR, IP2STR(&ip.ip));
    post_messages_t postMessage;

    int readyConnect = 0;
    uint8_t SSID[30] = {0};
    char Password[30] = {0};

    while (1)
    {

        // Waiting for submit
        if (xQueueReceive(xQueueHttp, &postMessage, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGW(TAG, "message: %.*s", postMessage.lenght,
                     postMessage.message);

            for (int i = 0; i < postMessage.lenght; i++)
            {
                if (postMessage.message[i] == '%')
                {
                    postMessage.message[i] =
                        (postMessage.message[i + 1] - '0') * 16 +
                        (postMessage.message[i + 2] - '0');

                    for (int l = i + 1; l < postMessage.lenght - 1; l++)
                    {
                        postMessage.message[l] = postMessage.message[l + 2];
                    }
                }
                else if (postMessage.message[i] == '+')
                {
                    postMessage.message[i] = ' ';
                }
            }

            int teste[2][2] = {0};
            int contador = 0;

            for (int i = 0; i < postMessage.lenght; i++)
            {
                if (postMessage.message[i] == '=')
                {
                    teste[0][contador] = i;
                    contador++;
                }

                if (postMessage.message[i] == '&')
                {
                    teste[1][0] = i;
                }
            }

            int ssidlenght = (teste[1][0] - teste[0][0]) - 1;
            int Passwordlenght = postMessage.lenght - teste[0][1] - 1;

            // pegar ssid
            for (int l = 0; l < ssidlenght; l++)
            {
                SSID[l] = postMessage.message[teste[0][0] + l + 1];
            }

            printf("ID = %s \n", SSID);

            // pegar password
            for (int l = 0; l < Passwordlenght; l++)
            {
                Password[l] = postMessage.message[teste[0][1] + l + 1];
            }

            printf("PW = %s \n", Password);

            readyConnect = 1;
        }

        if (readyConnect == 1)
        {
            
            init_wifi(SSID, Password);
            printf("\n\n CONECTADO \n\n");


            vTaskDelay(20000 / portTICK_PERIOD_MS);
            printf("\n\n DESLIGANDO AP \n\n");
            esp_wifi_stop();
            printf("\n\n AP DESLIGADO \n\n");
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            printf("\n\n LIGANDO AP \n\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            wifi_init_softap(0);
            printf("\n\n AP ONLINE  \n\n");
            // init_wifi(SSID, Password);
        }
    }
}
