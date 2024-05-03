#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "piezoSensor.h"
#include "capactiveSensor.h"
#include "presenceDetection.h"
#include "wakeupinator.h"
#include "alarm.h"
#include "button.h"
#include "wifi.h"
#include "mqttHandler.h"
#include "dataUpload.h"
#include "console.h"

#include "esp_log.h"

static TaskHandle_t s_task_handle;

void tempButtonCallback()
{
    // print the last capacityDiff
    ESP_LOGI("Button", "Last capacityDiff: %lu", getCapacityDiff());
    setAlarmState(ALARM_PRIMED);
}

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main() 
{
    initialize_nvs();

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

    // Initialize the presence detection
    initPresenceDetection();

    // Start the console task
    initConsole();

    while(1) {

        wifiTask();

        piezoSensorTask();

        capacitiveSensorTask();

        presenceDetectionTask();

        wakeupinatorTask();

        alarmTask();

        dataUploadTask();

        buttonTask();

        consoleTask();

        vTaskDelay(1);
    }
}