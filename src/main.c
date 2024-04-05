#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "piezoSensor.h"
#include "capactiveSensor.h"
#include "wakeupinator.h"
#include "alarm.h"
#include "button.h"
#include "wifi.h"
#include "mqttHandler.h"
#include "dataUpload.h"

#include "esp_log.h"

static TaskHandle_t s_task_handle;

void tempButtonCallback()
{
    // print the last capacityDiff
    ESP_LOGI("Button", "Last capacityDiff: %lu", getCapacityDiff());
    setAlarmState(ALARM_PRIMED);
}

void app_main() 
{

    initWifi();

    s_task_handle = xTaskGetCurrentTaskHandle();

    // Initialize the piezo sensor
    initPiezoSensor(&s_task_handle);

    // Initialize the capacitive sensor
    initCapacitiveSensor();

    // Initialize the wakeupinator
    initWakeupinator();

    initAlarm();
    // setAlarmState(ALARM_PRIMED);

    while (!isWifiConnected())
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    initMQTT();
    
    initDataUpload();

    initButton();
    setButtonPressedCallback(tempButtonCallback);

    while(1) {

        wifiTask();

        piezoSensorTask();

        capacitiveSensorTask();

        wakeupinatorTask();

        alarmTask();

        dataUploadTask();

        buttonTask();

        vTaskDelay(1);
    }
}