#include "capactiveSensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/queue.h"
#include "driver/gptimer.h"
#include "driver/pulse_cnt.h"
#include "soc/pcnt_struct.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "mqttHandler.h"
#include "secrets.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#define TAG "CapacitiveSensor"

// counter definitions
pcnt_unit_handle_t counterHandle;
volatile uint32_t overflowCount = 0;
pcnt_unit_config_t pcnt_config;
uint32_t lastCapacity = 0;
uint32_t capacityDiff = 0;

// timer definitions
gptimer_handle_t gptimer = NULL;
typedef struct
{
  uint64_t event_count;
} example_queue_element_t;
bool computeCapacityDiffFlag = false;

// calibration definitions
int64_t lastMovement = (int64_t)6.48e7;
int64_t lastCalibration = 0;
unsigned long highCapacity = 64000000 / 3 / 60;
unsigned long lowCapacity = 40000000 / 3 / 60;
unsigned long capacityDiffThreshold = 0;
unsigned long capacityDiffHysteresis = 0;
_Bool inBed = false;
int presenceUpdateCounter = 0;

void saveCalibrationValues()
{
  ESP_LOGI(TAG, "Saving calibration values: highCapacity: %lu, lowCapacity: %lu", highCapacity, lowCapacity);
  nvs_handle_t nvsHandle;
  ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));
  ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "highCapacity", highCapacity));
  ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "lowCapacity", lowCapacity));
  ESP_ERROR_CHECK(nvs_commit(nvsHandle));
  nvs_close(nvsHandle);
}

void updateInBedStatus()
{
  if (presenceUpdateCounter < 3)
  {
    presenceUpdateCounter++;
    return;
  }
  if (inBed)
  {
    if (capacityDiff > capacityDiffThreshold + capacityDiffHysteresis)
    {
      ESP_LOGI(TAG, "Out of bed");
      char payload[100];
      sprintf(payload, "{\"inBed\": false, \"capacity\":%lu}", getCapacityDiff());
      sendMqttData(payload, MQTT_TOPIC_WAKEUP, 1);
      inBed = false;
      saveCalibrationValues();
    }
  }
  else
  {
    if (capacityDiff < capacityDiffThreshold - capacityDiffHysteresis)
    {
      ESP_LOGI(TAG, "In bed");
      char payload[100];
      sprintf(payload, "{\"inBed\": true, \"capacity\":%lu}", getCapacityDiff());
      sendMqttData(payload, MQTT_TOPIC_WAKEUP, 1);
      inBed = true;
    }
  }
}

void computeCapacityDiff()
{
  if (computeCapacityDiffFlag)
  {
    computeCapacityDiffFlag = false;
    uint32_t capacity = getCapacitiveSensorValue();
    capacityDiff = capacity - lastCapacity;
    lastCapacity = capacity;
    updateInBedStatus();
  }
}

void computeThresholds()
{
  capacityDiffHysteresis = (highCapacity - lowCapacity) / 4;
  capacityDiffThreshold = (highCapacity + lowCapacity + capacityDiffHysteresis) / 2;
}

void updateCalibration()
{
  int64_t time_us = esp_timer_get_time();
  if (time_us - lastMovement > 1000000ULL * 60ULL * 60ULL * 2ULL)
  {
    if (!getInBedStatus() && time_us - lastCalibration > 1000000ULL * 60ULL * 60ULL)
    {
      highCapacity = (highCapacity * 3 + getCapacityDiff()) / 4;
      if (highCapacity > 65000000 / 3 / 60)
      {
        highCapacity = 65000000 / 3 / 60;
      }
      computeThresholds();
      lastCalibration = time_us;
    }
  }
  else if (time_us - lastMovement > 10000000)
  {
    if (getInBedStatus() && time_us - lastCalibration > 1000000ULL * 60ULL)
    {
      lowCapacity = (lowCapacity * 3 + getCapacityDiff()) / 4;
      if (lowCapacity < 40000000 / 3 / 60)
      {
        lowCapacity = 40000000 / 3 / 60;
      }
      computeThresholds();
      lastCalibration = time_us;
    }
  }
}

void notifyMovement()
{
  ESP_LOGI(TAG, "Movement detected");
  lastMovement = esp_timer_get_time();
}

static bool overflowHandler(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
  overflowCount++;
  return true;
}

void initPCNT()
{
  // setup the pulse counter
  pcnt_config.high_limit = 0x7fff;
  pcnt_config.low_limit = -0x8000;
  pcnt_config.intr_priority = 0;
  pcnt_config.flags.accum_count = false;
  // pcnt_config.flags.accum_count = true;
  ESP_ERROR_CHECK(pcnt_new_unit(&pcnt_config, &counterHandle));

  pcnt_chan_config_t chan_config = {
      .edge_gpio_num = pulsePin,
      .level_gpio_num = -1,
  };
  pcnt_channel_handle_t pcnt_chan = NULL;
  ESP_ERROR_CHECK(pcnt_new_channel(counterHandle, &chan_config, &pcnt_chan));

  ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));

  ESP_ERROR_CHECK(pcnt_unit_add_watch_point(counterHandle, pcnt_config.high_limit));

  // set overflow handler
  pcnt_event_callbacks_t cbs = {
      .on_reach = overflowHandler,
  };

  ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(counterHandle, &cbs, NULL));

  pcnt_glitch_filter_config_t glitch_filter_config = {
      .max_glitch_ns = 10,
  };
  ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(counterHandle, &glitch_filter_config));

  ESP_ERROR_CHECK(pcnt_unit_enable(counterHandle));
  ESP_ERROR_CHECK(pcnt_unit_start(counterHandle));
}

void initcapacitiveGPIO()
{
  gpio_config_t io_conf = {
      .pin_bit_mask = 1ULL << oscilatorPin,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&io_conf));
  ESP_ERROR_CHECK(gpio_set_level(oscilatorPin, 1));

  io_conf.pin_bit_mask = 1ULL << pulsePin;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&io_conf));
}

static bool capacityTimerHandler(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
  computeCapacityDiffFlag = true;
  return true;
}

void initDiffTimer()
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
      .on_alarm = capacityTimerHandler, // register user callback
  };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
  ESP_ERROR_CHECK(gptimer_enable(gptimer));
  ESP_ERROR_CHECK(gptimer_start(gptimer));
}

int initCapacitiveSensor()
{
  ESP_LOGI(TAG, "Starting capacitive sensor");

  // read the threshold values from the NVS
  nvs_handle_t nvsHandle;
  ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));
  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u32(nvsHandle, "highCapacity", &highCapacity));
  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u32(nvsHandle, "lowCapacity", &lowCapacity));
  nvs_close(nvsHandle);

  computeThresholds();

  capacityDiff = getCapacitythreshold() + 1000;

  ESP_LOGI(TAG, "highCapacity: %lu, lowCapacity: %lu, capacityDiffThreshold: %lu, capacityDiffHysteresis: %lu", highCapacity, lowCapacity, capacityDiffThreshold, capacityDiffHysteresis);

  initPCNT();
  initcapacitiveGPIO();
  initDiffTimer();

  return 0;
}

void capacitiveSensorTask()
{
  computeCapacityDiff();
  updateCalibration();
}

unsigned long getCapacityDiff()
{
  return capacityDiff;
}

unsigned long getCapacitiveSensorValue()
{
  int capacityAmount = 0;
  pcnt_unit_get_count(counterHandle, &capacityAmount);
  return capacityAmount + (overflowCount * pcnt_config.high_limit);
}

void resetCapacitiveSensorValue()
{
  overflowCount = 0;
  pcnt_unit_clear_count(counterHandle);
}

_Bool getInBedStatus()
{
  return inBed;
}

unsigned long getCapacitythreshold()
{
  return capacityDiffThreshold;
}