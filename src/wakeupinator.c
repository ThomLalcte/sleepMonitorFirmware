#include "wakeupinator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

bool wakeupinatorEnabled = false;
#define TAG "wakeupinator"

void initWakeupinatorGPIO()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << motorPin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(motorPin, 0));
}

void initWakeupinator()
{
    ESP_LOGI(TAG, "Starting wakeupinator");
    initWakeupinatorGPIO();
}

void enableWakeupinator()
{
    ESP_LOGV(TAG, "Enabling wakeupinator");
    wakeupinatorEnabled = true;
}

void disableWakeupinator()
{
    ESP_LOGV(TAG, "Disabling wakeupinator");
    wakeupinatorEnabled = false;
    ESP_ERROR_CHECK(gpio_set_level(motorPin, 0));
}

void wakeupinatorTask()
{
    if (wakeupinatorEnabled)
    {
        ESP_ERROR_CHECK(gpio_set_level(motorPin, (esp_timer_get_time() / 1000)%cycleTime < cycleTime/2));
    }
}