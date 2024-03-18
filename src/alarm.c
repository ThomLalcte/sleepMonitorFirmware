#include "alarm.h"
#include "stdbool.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#define TAG "alarm"

enum alarmState_t alarmState = ALARM_OFF;

unsigned long capactityDiff = 0;
unsigned long initCapactityDiff = 0;
unsigned long lastCapacity = 0;
unsigned long outThreshold = 10000;
bool checkCapacitiveSensorFlag = false;

unsigned int timerCounter = 0;
gptimer_handle_t gptimer = NULL;
typedef struct
{
    uint64_t event_count;
} example_queue_element_t;

void setAlarmState(enum alarmState_t state)
{
    alarmState = state;
    ESP_LOGI(TAG, "Alarm State: %d", state);
}

void setThreshold(unsigned long threshold)
{
    outThreshold = threshold;
    ESP_LOGI(TAG, "Threshold set to: %lu", threshold);
}

static bool example_timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    checkCapacitiveSensorFlag = true;
    timerCounter++;
    return true;
}

void initAlarmTimer()
{
    ESP_LOGI(TAG, "Initializing Alarm Timer");
    // Initialize the alarm timer
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,                  // counter will reload with 0 on alarm event
        .alarm_count = 1000000,             // period = 1s @resolution 1MHz
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = example_timer_on_alarm_cb, // register user callback
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

void initAlarm()
{
    ESP_LOGI(TAG, "Initializing Alarm");
    initAlarmTimer();
}

void alarmTask()
{
    switch (alarmState)
    {
    case ALARM_OFF:
        break;
    case ALARM_PRIMED:
        if (checkCapacitiveSensorFlag)
        {
            initCapactityDiff = capactityDiff;
            alarmState = ALARM_ON;
            enableWakeupinator();
        }
        break;
    case ALARM_ON:
        ESP_LOGI(TAG, "Change since set: %lu", capactityDiff - initCapactityDiff);
        if ((capactityDiff - initCapactityDiff) > outThreshold)
        {
            disableWakeupinator();
            alarmState = ALARM_OFF;
        }
        if (timerCounter > 60)
        {
            alarmState = ALARM_FAILED;
            disableWakeupinator();
        }
        break;
    case ALARM_FAILED:
        // contact the server and report the failure
        alarmState = ALARM_OFF;
        break;
    }

    // if (checkCapacitiveSensorFlag && (alarmState == ALARM_PRIMED || alarmState == ALARM_ON))
    if (checkCapacitiveSensorFlag)
    {
        sampleCapacitiveDiff();
        checkCapacitiveSensorFlag = false;
    }
}

void sampleCapacitiveDiff()
{
    unsigned long currentCapacity = getCapacitiveSensorValue();
    capactityDiff = currentCapacity - lastCapacity;
    // ESP_LOGI(TAG, "currentCapacity: %lu", currentCapacity);
    // ESP_LOGI(TAG, "lastCapacity: %lu", lastCapacity);
    // ESP_LOGI(TAG, "capactityDiff: %lu", capactityDiff);
    lastCapacity = currentCapacity;
}