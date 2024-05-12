#include "wakeupinator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

#define TAG "wakeupinator"

_Bool wakeupinatorEnabled = false;

// Intensity of the wakeupinator: 0 = vibrations only, 1 = vibrations and sound
unsigned short vibecheckLevel = 0;
_Bool state = false;

int64_t nextRunTime = 0;

// set the intensity of the wakeupinator: 0 = vibrations only, 1 = vibrations and sound
void setVibecheckLevel(const unsigned short level)
{
    if (level > 1)
    {
        vibecheckLevel = 1;
    }
    else
    {
        vibecheckLevel = level;
    }
    ESP_LOGI(TAG, "Vibecheck level set to %d", vibecheckLevel);
}

void initWakeupinatorGPIO()
{
    ESP_LOGI(TAG, "Initializing GPIO");
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << motorPin | 1ULL << buzzPinA,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(motorPin, 0));
    ESP_ERROR_CHECK(gpio_set_level(buzzPinA, 0));
    // ESP_ERROR_CHECK(gpio_set_level(buzzPinB, 0));
}

void initLedc()
{
    ESP_LOGI(TAG, "Initializing LEDC");
    ledc_timer_config_t ledcTimer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 869,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledcTimer));

    ledc_channel_config_t ledcChannel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = buzzPinB,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledcChannel));
    ESP_LOGI(TAG, "Done initializing LEDC");
}

void initWakeupinator()
{
    ESP_LOGI(TAG, "Starting wakeupinator");
    initWakeupinatorGPIO();
    initLedc();
}

void enableWakeupinator()
{
    ESP_LOGI(TAG, "Enabling wakeupinator");
    wakeupinatorEnabled = true;
    nextRunTime = esp_timer_get_time() + cycleTime * 1000;
}

void disableWakeupinator()
{
    ESP_LOGI(TAG, "Disabling wakeupinator");
    wakeupinatorEnabled = false;
    ESP_ERROR_CHECK(gpio_set_level(motorPin, 0));
    // ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
    // ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    vibecheckLevel = 0;
}

void wakeupinatorTask()
{
    if (wakeupinatorEnabled)
    {
        if (esp_timer_get_time() > nextRunTime)
        {
            nextRunTime = esp_timer_get_time() + cycleTime * 1000;
            if (state)
            {
                // ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
                ESP_ERROR_CHECK(gpio_set_level(motorPin, 0));
            }
            else
            {
                switch (vibecheckLevel)
                {
                case 0:
                    // ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
                    ESP_ERROR_CHECK(gpio_set_level(motorPin, 1));
                    break;
                case 1:
                    // ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1 << 9));
                    ESP_ERROR_CHECK(gpio_set_level(motorPin, 1));
                    break;
                }
            }
        }
        state = !state;
        // ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    }
}