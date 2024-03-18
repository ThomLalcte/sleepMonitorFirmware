#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "piezoSensor.h"
#include "capactiveSensor.h"
#include "wakeupinator.h"

static TaskHandle_t s_task_handle;

void app_main() 
{

    s_task_handle = xTaskGetCurrentTaskHandle();

    // Initialize the piezo sensor
    initPiezoSensor(&s_task_handle);

    // Initialize the capacitive sensor
    initCapacitiveSensor();

    // Initialize the wakeupinator
    initWakeupinator();

    while(1) {
        piezoSensorTask();

        capacitiveSensorTask();

        wakeupinatorTask();

        vTaskDelay(1);
    }
}