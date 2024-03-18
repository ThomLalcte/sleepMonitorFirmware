#include "alarm.h"
#include "stdbool.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#define TAG "alarm"

enum alarmState_t alarmState = ALARM_OFF;

long capactityDiff = 0;
long initCapactityDiff = 0;
unsigned long lastCapacity = 0;
long outThreshold = 1000;
bool checkCapacitiveSensorFlag = false;

unsigned int timerCounter = 0;
gptimer_handle_t gptimer = NULL;
typedef struct
{
    uint64_t event_count;
} example_queue_element_t;
bool timerState = false;

void setAlarmState(enum alarmState_t state)
{
    alarmState = state;
    ESP_LOGI(TAG, "Alarm State: %d", state);
}

int getAlarmState()
{
    return alarmState;
}

void setThreshold(long threshold)
{
    outThreshold = threshold;
    ESP_LOGI(TAG, "Threshold set to: %lu", threshold);
}

long getThreshold()
{
    return outThreshold;
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
    timerState = true;
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
        if (!timerState)
        {
            timerState = true;
            ESP_ERROR_CHECK(gptimer_start(gptimer));
        }
        if (checkCapacitiveSensorFlag)
        {
            initCapactityDiff = capactityDiff;
            alarmState = ALARM_ON;
            enableWakeupinator();
            ESP_LOGI(TAG, "Alarm primed with initial capacity: %lu", initCapactityDiff);
        }
        break;
    case ALARM_ON:
        if (checkCapacitiveSensorFlag)
        {
            long changeCapacity = capactityDiff - initCapactityDiff;
            ESP_LOGI(TAG, "Change since set: %ld", changeCapacity);
            if (changeCapacity > outThreshold)
            {
                disableWakeupinator();
                alarmState = ALARM_OFF;
                if (timerState)
                {
                    timerState = false;
                    ESP_ERROR_CHECK(gptimer_stop(gptimer));
                }
            }
        }
        if (timerCounter > 300)
        {
            alarmState = ALARM_FAILED;
            disableWakeupinator();
            if (timerState)
            {
                timerState = false;
                ESP_ERROR_CHECK(gptimer_stop(gptimer));
            }
        }
        break;
    case ALARM_FAILED:
        // contact the server and report the failure
        alarmState = ALARM_OFF;
        break;
    case ALARM_PREWAKE:
        alarmState = ALARM_OFF;
        break;
    default:
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
    capactityDiff = (long)(currentCapacity - lastCapacity);
    // ESP_LOGI(TAG, "currentCapacity: %lu", currentCapacity);
    // ESP_LOGI(TAG, "lastCapacity: %lu", lastCapacity);
    // ESP_LOGI(TAG, "capactityDiff: %lu", capactityDiff);
    lastCapacity = currentCapacity;
}