#include "alarm.h"
#include "stdbool.h"
#include "esp_log.h"
#include "capactiveSensor.h"
#include "presenceDetection.h"
#include "esp_timer.h"
#include "mqttHandler.h"
#include "secrets.h"
#include "string.h"
#include "timeSync.h"
#include "nvs_flash.h"

#define TAG "alarm"

enum alarmState_t alarmState = ALARM_OFF;
int64_t startTime = 0;
int64_t lastRunTime = 0;

int64_t nextAlarm = 0;
unsigned char nextAlarmStrengh = 0;

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
    if (strcmp(payloadCopy, ALARM_ON_PAYLOAD) == 0)
    {
        setAlarmState(ALARM_PRIMED);
        ESP_LOGI(TAG, "Alarm primed: threshold= %lu, lastDiff=%lu", getCapacitythreshold(), getCapacityDiff());
    }
    else if (strcmp(payloadCopy, ALARM_OFF_PAYLOAD) == 0)
    {
        disableWakeupinator();
        setAlarmState(ALARM_OFF);
        ESP_LOGI(TAG, "Alarm off");
    }
    else if (strcmp(payloadCopy, ALARM_PREWAKE_PAYLOAD) == 0)
    {
        setAlarmState(ALARM_PREWAKE);
        ESP_LOGI(TAG, "Alarm prewake");
    }
    // check if the payload is a number
    // if it is, set the vibecheck level according to the last character of the payload
    else if (payloadCopy[0] >= '0' && payloadCopy[0] <= '9')
    {
        // nextAlarm = atoll(payloadCopy);
        // split the payload into two parts
        char strength = payloadCopy[strlen(payloadCopy) - 1];
        char *time = malloc(strlen(payloadCopy));
        strncpy(time, payloadCopy, strlen(payloadCopy) - 1);
        nextAlarm = atoll(time);
        if (strength == 's')
        {
            nextAlarmStrengh = 1;
        }
        else if (strength == 'w')
        {
            nextAlarmStrengh = 0;
        }
        else
        {
            ESP_LOGE(TAG, "Invalid strengh character: %c", strength);
        }
        free(time);
        ESP_LOGI(TAG, "Next alarm: %llu with strength: %d", nextAlarm, nextAlarmStrengh);
        // save the next alarm to the nvs
        nvs_handle_t nvsHandle;
        ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));
        ESP_ERROR_CHECK(nvs_set_i64(nvsHandle, NEXT_ALARM_NVS_KEY, nextAlarm));
        ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, NEXT_ALARM_STRENGTH_NVS_KEY, nextAlarmStrengh));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(nvsHandle));
        nvs_close(nvsHandle);
    }
    else
    {
        ESP_LOGE(TAG, "Invalid payload: %s", payloadCopy);
    }
    free(payloadCopy);
}

void fetchLastNextAlarm()
{
    nvs_handle_t nvsHandle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_i64(nvsHandle, NEXT_ALARM_NVS_KEY, &nextAlarm));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u8(nvsHandle, NEXT_ALARM_STRENGTH_NVS_KEY, &nextAlarmStrengh));
    nvs_close(nvsHandle);
    ESP_LOGI(TAG, "Next alarm: %llu with strength: %d", nextAlarm, nextAlarmStrengh);
}

int64_t getNextAlarm()
{
    return nextAlarm;
}

unsigned char getNextAlarmStrengh()
{
    return nextAlarmStrengh;
}

void initAlarm()
{
    ESP_LOGI(TAG, "Initializing Alarm");
    nextAlarm = getTime() + 60 * 60 * 24;
    fetchLastNextAlarm();
    subscribeToTopic(MQTT_TOPIC_ALARM, alarmCallback);
    ESP_LOGI(TAG, "Done initializing Alarm");
}

void alarmTask()
{
    int64_t time_us = esp_timer_get_time();
    if ((time_us - lastRunTime) > 100000)
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
        if (nextAlarm < getTime())
        {
            if (nextAlarmStrengh == 0)
            {
                alarmState = ALARM_PRIMED;
            }
            else
            {
                alarmState = ALARM_PRIMED_PREWAKE;
            }
            nextAlarm += 60 * 60 * 24;
        }
        break;
    case ALARM_PRIMED:
        if (getInBedStatus())
        // if (1)
        {
            alarmState = ALARM_ON;
            startTime = time_us;
            enableWakeupinator();
            setVibecheckLevel(0);
            ESP_LOGI(TAG, "Alarm primed with startTime: %llu", startTime);
        }
        else
        {
            alarmState = ALARM_OFF;
            disableWakeupinator();
        }
        break;
    case ALARM_ON:
        // ESP_LOGI(TAG, "Alarm on for %llu seconds", (time_us - startTime) / 1000000);
        if (!getInBedStatus())
        {
            alarmState = ALARM_OFF;
            disableWakeupinator();
        }
        if ((time_us - startTime) > (120 * 1000000))
        {
            setVibecheckLevel(1);
            alarmState = ALARM_MORE_ON;
        }
        break;
    case ALARM_MORE_ON:
        ESP_LOGI(TAG, "Alarm on for %llu seconds", (time_us - startTime) / 1000000);
        if (!getInBedStatus())
        {
            alarmState = ALARM_OFF;
            disableWakeupinator();
        }
        if ((time_us - startTime) > (240 * 1000000))
        {
            alarmState = ALARM_FAILED;
            disableWakeupinator();
            ESP_LOGI(TAG, "Alarm timed out after 240 seconds");
        }
        break;
    case ALARM_FAILED:
        // contact the server and report the failure
        sendMqttData("alarm failed", MQTT_TOPIC_ALARM, 0);
        alarmState = ALARM_OFF;
        break;
    case ALARM_PRIMED_PREWAKE:
        if (getInBedStatus())
        {
            alarmState = ALARM_PREWAKE;
            startTime = time_us;
            enableWakeupinator();
            setVibecheckLevel(0);
            ESP_LOGI(TAG, "Alarm primed with startTime: %llu", startTime);
        }
        alarmState = ALARM_OFF;
        break;
    case ALARM_PREWAKE:
        if ((time_us - startTime) > (10 * 1000000))
        {
            alarmState = ALARM_OFF;
            disableWakeupinator();
        }
        break;
    default:
        alarmState = ALARM_OFF;
        break;
    }
}
