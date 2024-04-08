#include "alarm.h"
#include "stdbool.h"
#include "esp_log.h"
#include "capactiveSensor.h"
#include "esp_timer.h"
#include "mqttHandler.h"
#include "secrets.h"
#include "string.h"

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

void alarmCallback(char *payload, int payloadLength, char *topic)
{
    ESP_LOGI(TAG, "Alarm callback");
    char *payloadCopy = malloc(payloadLength + 1);
    memcpy(payloadCopy, payload, payloadLength);
    payloadCopy[payloadLength] = '\0';
    ESP_LOGI(TAG, "PayloadCopy: %s with lenght: %d", payloadCopy, payloadLength); // looks like the payload is not null terminated
    if (strcmp(payloadCopy, "on") == 0)
    {
        setAlarmState(ALARM_PRIMED);
        ESP_LOGI(TAG, "Alarm primed: threshold= %lu, lastDiff=%lu", getCapacitythreshold(), getCapacityDiff());
    }
    else if (strcmp(payloadCopy, "off") == 0)
    {
        setAlarmState(ALARM_OFF);
        disableWakeupinator();
    }
    free(payloadCopy);
}

void initAlarm()
{
    ESP_LOGI(TAG, "Initializing Alarm");
    subscribeToTopic(MQTT_TOPIC_ALARM, alarmCallback);
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
        if (getInBedStatus())
        {
            alarmState = ALARM_ON;
            startTime = time_us;
            enableWakeupinator();
            ESP_LOGI(TAG, "Alarm primed with startTime: %llu", startTime);
        }
        break;
    case ALARM_ON:
        ESP_LOGI(TAG, "Alarm on for %llu seconds", (time_us - startTime) / 1000000);
        if (!getInBedStatus())
        {
            alarmState = ALARM_OFF;
            disableWakeupinator();
        }
        if ((time_us - startTime) > (180 * 1000000))
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
