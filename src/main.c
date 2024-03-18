#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "piezoSensor.h"
#include "capactiveSensor.h"
#include "wakeupinator.h"
#include "alarm.h"
#include "button.h"

#include "esp_log.h"

static TaskHandle_t s_task_handle;

void tempButtonCallback()
{
    setAlarmState(ALARM_PRIMED);
}

void app_main() 
{

    s_task_handle = xTaskGetCurrentTaskHandle();

    // Initialize the piezo sensor
    initPiezoSensor(&s_task_handle);

    // Initialize the capacitive sensor
    initCapacitiveSensor();

    // Initialize the wakeupinator
    initWakeupinator();

    initAlarm();
    // setAlarmState(ALARM_PRIMED);
    setThreshold(10000);

    initButton();
    setButtonPressedCallback(tempButtonCallback);

    while(1) {
        piezoSensorTask();

        capacitiveSensorTask();

        wakeupinatorTask();

        alarmTask();

        buttonTask();

        vTaskDelay(1);
    }
}