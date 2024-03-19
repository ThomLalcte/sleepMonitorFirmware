#include "secrets.h"

#include <string.h>            //for handling strings
#include "freertos/FreeRTOS.h" //for delay,mutexs,semphrs rtos operations
#include "esp_system.h"        //esp_init funtions esp_err_t
#include "esp_wifi.h"          //esp_wifi_init functions and wifi operations
#include "esp_log.h"           //for showing logs
#include "esp_event.h"         //for wifi event
#include "nvs_flash.h"         //non volatile storage
#include "lwip/err.h"          //light weight ip packets error handling
#include "lwip/sys.h"          //system applications for light weight ip apps

#include <sys/socket.h>

int retry_num = 0;

char pingedServer = 0;

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
        pingedServer = 1;
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

void tcp_client(void)
{
    char tx_buffer[] = "ping";
    char rx_buffer[128];
    int addr_family = 0;
    int ip_protocol = 0;

    struct sockaddr_in dest_addr;
    inet_pton(AF_INET, host, &dest_addr.sin_addr);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host, port);

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        return;
    }

    ESP_LOGI(TAG, "Successfully connected");

    while (1)
    {
        int err = send(sock, tx_buffer, strlen(tx_buffer), 0);
        if (err < 0)
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
        }

        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        // Error occurred during receiving
        if (len < 0)
        {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        }
        // Data received
        else
        {
            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
            break;
        }
    }
}

void initWifi()
{
    ESP_LOGI(TAG, "Initializing Wifi");
    nvs_flash_init();
    wifi_connection();
}

void wifiTask() {
    if (pingedServer == 1) {
        pingedServer = 2;
        tcp_client();
    }
}