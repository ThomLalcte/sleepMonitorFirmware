#include "capactiveSensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/queue.h"
// #include "driver/gptimer.h"
#include "driver/pulse_cnt.h"
#include "soc/pcnt_struct.h"
#include "driver/gpio.h"
#include "esp_log.h"

// counter definitions
pcnt_unit_handle_t counterHandle;
volatile uint32_t overflowCount = 0;
pcnt_unit_config_t pcnt_config;

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

int initCapacitiveSensor()
{
  ESP_LOGI(TAG, "Starting capacitive sensor");

  initPCNT();
  initcapacitiveGPIO();

  return 0;
}

void capacitiveSensorTask()
{
  vTaskDelay(1);
}

unsigned long getCapacitiveSensorValue() {
  int capacityAmount = 0;
  pcnt_unit_get_count(counterHandle, &capacityAmount);
  return capacityAmount + (overflowCount * pcnt_config.high_limit);
}