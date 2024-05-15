#include "dataUpload.h"

#include "esp_log.h"
#include "driver/gptimer.h"

#include "mqttHandler.h"
#include "capactiveSensor.h"
#include "piezoSensor.h"
#include "secrets.h"

#define TAG "dataUpload"

volatile bool sendData = 0;
gptimer_handle_t uploadTimerHandle = NULL;

static bool IRAM_ATTR uploadTimerCompareCallback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    sendData = true;
    return false;
}

void initSendTimer()
{
    ESP_LOGI(TAG, "Initializing dataUpload Timer");
    // Initialize the alarm timer
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &uploadTimerHandle));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,                  // counter will reload with 0 on alarm event
        .alarm_count = 1000000 * 60 * 3,    // period = 1s @resolution 1MHz
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(uploadTimerHandle, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = uploadTimerCompareCallback, // register user callback
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(uploadTimerHandle, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(uploadTimerHandle));
    ESP_ERROR_CHECK(gptimer_start(uploadTimerHandle));
}

void uploadSensorData()
{
    ESP_LOGI(TAG, "Uploading sensor data");

    unsigned long capacityReading = getCapacitiveSensorValue();
    resetCapacitiveSensorValue();

    unsigned long piezoReading = getPiezoSensorValue();
    resetPiezoSensorValue();

    // format the data
    char data[] = "{\"type\":\"%s\",\"value\":%lu,\"datatype\":\"int\"}";
    char dataBuffer[256];
    sprintf(dataBuffer, data, "capacity", capacityReading);
    sendMqttData(dataBuffer, MQTT_TOPIC_CAPACITY, 0);

    sprintf(dataBuffer, data, "piezo", piezoReading);
    sendMqttData(dataBuffer, MQTT_TOPIC_PIEZO, 0);

    sendData = 0;
}

void initDataUpload()
{
    initSendTimer();

    sendData = 0;
}

void dataUploadTask()
{
    if (sendData)
    {
        uploadSensorData();
    }
}