#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

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
    ESP_ERROR_CHECK(nvs_flash_init());

    initWifi();

    s_task_handle = xTaskGetCurrentTaskHandle();

    // Initialize the piezo sensor
    initPiezoSensor(&s_task_handle);



    while (!isWifiConnected())
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    initMQTT();
    
    initDataUpload();
    
    // Initialize the wakeupinator
    initWakeupinator();

    initAlarm();
    // setAlarmState(ALARM_PRIMED);

    initButton();
    setButtonPressedCallback(tempButtonCallback);
    
    // Initialize the capacitive sensor
    initCapacitiveSensor();

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