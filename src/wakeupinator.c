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
int64_t motorLastStartTime = 0; // time between vibrations in ms
int64_t motorLastStopTime = 0;  // time between vibrations in ms
int64_t motorOntime = 0;  // watchdog for motor
int64_t motorOfftime = 0;  // watchdog for motor

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

    motorLastStartTime = esp_timer_get_time();
    motorLastStopTime = esp_timer_get_time();

    ESP_LOGI(TAG, "Done starting wakeupinator");
}

void enableSound()
{
    ESP_ERROR_CHECK(ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1 << 9));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

void disableSound()
{
    ESP_ERROR_CHECK(ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}


void enableMotor()
{
    motorLastStartTime = esp_timer_get_time();
    // motorOntime += (motorLastStartTime - motorLastStopTime)/1000;
    ESP_ERROR_CHECK(gpio_set_level(motorPin, 1));
}

void disableMotor()
{
    motorLastStopTime = esp_timer_get_time();
    // motorOfftime += (motorLastStopTime - motorLastStartTime)/1000;
    // motorOfftime += min(60000, motorOntime);
    ESP_ERROR_CHECK(gpio_set_level(motorPin, 0));
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
    disableMotor();
    disableSound();
    vibecheckLevel = 0;
}

void wakeupinatorTask()
{
    int64_t time_us = esp_timer_get_time();

    // if ((motorOntime - motorOfftime > 60000) && wakeupinatorEnabled)
    // {
    //     ESP_LOGI(TAG, "motor was on for too long, disabling wakeupinator (%lld & %lld)", motorOntime, motorOfftime);
    //     motorOntime = 0;
    //     motorOfftime = 0;
    //     disableWakeupinator();
    // }

    if ((time_us - motorLastStartTime) > 5LL * 60LL * 1000000LL && motorLastStartTime > motorLastStopTime)
    {
        ESP_LOGI(TAG, "motor was on for too long, disabling wakeupinator (%lld & %lld)", motorLastStartTime, motorLastStopTime);
        disableWakeupinator();
    }

    if (wakeupinatorEnabled)
    {
        if (time_us > nextRunTime)
        {
            nextRunTime = time_us + cycleTime * 1000;
            if (state)
            {
                disableMotor();
                disableSound();
            }
            else
            {
                switch (vibecheckLevel)
                {
                case 0:
                    enableMotor();
                    break;
                case 1:
                    enableSound();
                    enableMotor();
                    break;
                }
            }
        }
        state = !state;
    }
}