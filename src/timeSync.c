#include "timeSync.h"

#include "esp_sntp.h"
#include "stdint.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"

int64_t getTime()
{
    time_t now;
    time(&now);
    return now;
}

void initializeSNTP()
{
    ESP_LOGI("SNTP", "Initializing SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);
    
    // wait 10s for time to be set
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)));

    ESP_LOGI("SNTP", "SNTP initialized");
}