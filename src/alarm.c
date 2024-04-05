#include "alarm.h"
#include "stdbool.h"
#include "esp_log.h"
#include "capactiveSensor.h"
#include "esp_timer.h"

#define TAG "alarm"

enum alarmState_t alarmState = ALARM_OFF;
int64_t startTime = 0;
int64_t lastRunTime = 0;

void setAlarmState(enum alarmState_t state)
{
    alarmState = state;
    ESP_LOGI(TAG, "Alarm State: %d", state);
}

int getAlarmState()
{
    return alarmState;
}

void initAlarm()
{
    ESP_LOGI(TAG, "Initializing Alarm");
    ESP_LOGI(TAG, "Done initializing Alarm");
}

void alarmTask()
{
    int64_t time_us = esp_timer_get_time();
    if ((time_us - lastRunTime) > 1000000)
    {
        lastRunTime = time_us;
    }
    else
    {
        return;
    }

    switch (alarmState)
    {
    case ALARM_OFF:
        break;
    case ALARM_PRIMED:
        if (getTresholdCrossed())
        {
            alarmState = ALARM_ON;
            startTime = time_us;
            enableWakeupinator();
            ESP_LOGI(TAG, "Alarm primed with startTime: %llu", startTime);
        }
        break;
    case ALARM_ON:
        ESP_LOGI(TAG, "Alarm on for %llu seconds", (time_us - startTime) / 1000000);
        if (!getTresholdCrossed())
        {
            alarmState = ALARM_PREWAKE;
            disableWakeupinator();
        }
        if ((time_us - startTime) > (180*1000000))
        {
            alarmState = ALARM_FAILED;
            disableWakeupinator();
            ESP_LOGI(TAG, "Alarm timed out after 180 seconds");
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
}
