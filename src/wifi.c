#include "secrets.h"

#include <string.h>            //for handling strings
#include "freertos/FreeRTOS.h" //for delay,mutexs,semphrs rtos operations
#include "esp_system.h"        //esp_init funtions esp_err_t
#include "esp_wifi.h"          //esp_wifi_init functions and wifi operations
#include "esp_log.h"           //for showing logs
#include "esp_event.h"         //for wifi event
#include "lwip/err.h"          //light weight ip packets error handling
#include "lwip/sys.h"          //system applications for light weight ip apps

int retry_num = 0;
bool wifiConnected = 0;

#define TAG "wifi"

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "Wifi connecting");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "Wifi connected");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "Wifi lost connection");
        if (retry_num < 5)
        {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Retrying to connect");
        }
        else if (retry_num == 5)
        {
            ESP_LOGI(TAG, "Failed to connect to wifi, abandoning retry attempts. Please check your wifi credentials and try again.");
            retry_num++;
        }
        break;
    case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Wifi got IP");
        wifiConnected = 1;
        break;
    default:
        break;
    }
}

void wifi_connection()
{
    //                          s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_netif_init();
    esp_event_loop_create_default();     // event loop                    s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station                      s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,

        }};
    strcpy((char *)wifi_configuration.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_configuration.sta.password, WIFI_PASSWORD);
    // esp_log_write(ESP_LOG_INFO, "Kconfig", "SSID=%s, PASS=%s", ssid, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s  password:%s", WIFI_SSID, WIFI_PASSWORD);
}

void initWifi()
{
    ESP_LOGI(TAG, "Initializing Wifi");
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_connection();
}

void wifiTask() {
    // if (pingedServer == 1) {
    //     pingedServer = 2;
    //     tcp_client();
    // }
}

_Bool isWifiConnected()
{
    return wifiConnected;
}