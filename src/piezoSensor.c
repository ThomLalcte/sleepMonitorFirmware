#include "piezoSensor.h"
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "driver/gpio.h"
#include "math.h"

#define TAG "piezoSensor"

struct PiezoParameters_t
{
    short attenuation;   // log10 of the attenuation divided by 10meg (-1 = 1meg, -2 = 100k...)
    unsigned short gain; // log10 of the gain of the amplifier
} piezoParameters = {
    .attenuation = 0,
    .gain = 1,
};
adc_continuous_handle_t handle = NULL;
uint16_t piezoSampleBuffer[piezoSampleBufferSize];
bool checkReadingsFlag = false;
static TaskHandle_t s_task_handle;

static unsigned long long piezoSensorValue = 0;

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    checkReadingsFlag = true;
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);
    return mustYield == pdTRUE;
}

void initADC()
{
    // setup the adc
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = piezoSampleBufferSize,
        .conv_frame_size = convFrameSize,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_digi_pattern_config_t adc_digi_pattern_config = {
        .atten = ADC_ATTEN_DB_11,
        .channel = ADC_CHANNEL_0,
        .unit = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_10,
    };

    const adc_continuous_config_t digi_adc_config = {
        .pattern_num = 1,
        .adc_pattern = &adc_digi_pattern_config,
        .sample_freq_hz = piezoSamplingFrequency,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,

    };

    ESP_ERROR_CHECK(adc_continuous_config(handle, &digi_adc_config));

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
        .on_pool_ovf = NULL,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));

    ESP_ERROR_CHECK(adc_continuous_start(handle));
}

void initPiezoGPIOs()
{
    // setup the pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIEZO_G10_PIN) | (1ULL << PIEZO_ATN1MEG_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(PIEZO_G10_PIN, 1));
    ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 0));
    io_conf.pin_bit_mask = (1ULL << PIEZO_ATN100K_PIN) | (1ULL << PIEZO_ATN10K_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLDOWN_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
}

void initPiezoSensor(TaskHandle_t *mainTaskHandle)
{
    ESP_LOGI(TAG, "Initializing piezo sensor");

    unsigned long long piezoValue = 0;
    ESP_LOGI(TAG, "Max piezoValue: %llu", piezoValue-1);

    s_task_handle = *mainTaskHandle;

    initADC();

    initPiezoGPIOs();
}


void piezoSensorTask()
{
    if (checkReadingsFlag)
    {
        checkReadingsFlag = false;
        // ESP_LOGI(TAG, "Reading piezo sensor");
        readPiezoSensor();
    }
}

unsigned char sillyCounter = 0;

void readPiezoSensor()
{
    uint32_t readCount = 0;
    adc_continuous_read(handle, (uint8_t *)piezoSampleBuffer, piezoSampleBufferSize, &readCount, 50);
    // sum the values to reduce the size of the data
    unsigned long bufferSum = 0;
    for (int i = 0; i < sizeof(piezoSampleBuffer) / sizeof(piezoSampleBuffer[0]); i++)
    {
        bufferSum += piezoSampleBuffer[i];
    }
    if (bufferSum < 100)
    {
        increasePiezoSensitivity();
    }
    else if (bufferSum > (sizeof(piezoSampleBuffer) / sizeof(piezoSampleBuffer[0])) * (0b1<<(10-4)))
    {
        decreasePiezoSensitivity();
    }
    // sillyCounter++;
    if (sillyCounter%100 == 1)
    {
        ESP_LOGI(TAG, "Piezo sensor value: %lu", bufferSum);
        sillyCounter = 0;
    }
    bufferSum *= pow10(-piezoParameters.attenuation);
    bufferSum *= pow10(piezoParameters.gain);
    piezoSensorValue += bufferSum;
}

void increasePiezoSensitivity()
{
    // ESP_LOGI(TAG, "Increasing sensitivity");
    // check if the attenuation can be diminished
    if (piezoParameters.attenuation < 0)
    {
        piezoParameters.attenuation++;
        ESP_LOGV(TAG, "Attenuation: %d", piezoParameters.attenuation);
        switch (piezoParameters.attenuation)
        {
        case 0:
            ESP_LOGV(TAG, "Attenuation: 0");
            ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 0));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLDOWN_ONLY));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
            break;
        case -1:
            ESP_LOGV(TAG, "Attenuation: -1");
            ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 1));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLDOWN_ONLY));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
            break;
        case -2:
            ESP_LOGV(TAG, "Attenuation: -2");
            ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 1));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLUP_ONLY));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
            break;
        case -3:
            ESP_LOGV(TAG, "Attenuation: -3");
            ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 1));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLUP_ONLY));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLUP_ONLY));
            break;
        }
    }
    // check if the gain can be increased
    else if (piezoParameters.gain < 1)
    {
        piezoParameters.gain++;
        ESP_LOGV(TAG, "Gain: %d", piezoParameters.gain);
        ESP_ERROR_CHECK(gpio_set_level(PIEZO_G10_PIN, 1));
    }
}

void decreasePiezoSensitivity()
{
    // ESP_LOGI(TAG, "Decreasing sensitivity");
    // check if the gain can be diminished
    if (piezoParameters.gain > 0)
    {
        piezoParameters.gain--;
        ESP_LOGV(TAG, "Gain: %d", piezoParameters.gain);
        ESP_ERROR_CHECK(gpio_set_level(PIEZO_G10_PIN, 0));
    }
    // check if the attenuation can be increased
    else if (piezoParameters.attenuation > -3)
    {
        piezoParameters.attenuation--;
        ESP_LOGV(TAG, "Attenuation: %d", piezoParameters.attenuation);
        switch (piezoParameters.attenuation)
        {
        case 0:
            ESP_LOGV(TAG, "Attenuation: 0");
            ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 0));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLDOWN_ONLY));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
            break;
        case -1:
            ESP_LOGV(TAG, "Attenuation: -1");
            ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 1));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLDOWN_ONLY));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
            break;
        case -2:
            ESP_LOGV(TAG, "Attenuation: -2");
            ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 1));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLUP_ONLY));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
            break;
        case -3:
            ESP_LOGV(TAG, "Attenuation: -3");
            ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 1));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLUP_ONLY));
            ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLUP_ONLY));
            break;
        }
    }
}

void setPiezoSensitivity(short attenuation, unsigned short gain)
{
    if (attenuation < -3 || attenuation > 0)
    {
        ESP_LOGE(TAG, "Attenuation out of range");
        return;
    }
    if (gain > 1)
    {
        ESP_LOGE(TAG, "Gain out of range");
        return;
    }
    piezoParameters.attenuation = attenuation;
    piezoParameters.gain = gain;
    ESP_LOGI(TAG, "Attenuation: %d, Gain: %d", piezoParameters.attenuation, piezoParameters.gain);
    switch (piezoParameters.attenuation)
    {
    case 0:
        ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 0));
        ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLDOWN_ONLY));
        ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
        break;
    case -1:
        ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 1));
        ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLDOWN_ONLY));
        ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
        break;
    case -2:
        ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 1));
        ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLUP_ONLY));
        ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLDOWN_ONLY));
        break;
    case -3:
        ESP_ERROR_CHECK(gpio_set_level(PIEZO_ATN1MEG_PIN, 1));
        ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN100K_PIN, GPIO_PULLUP_ONLY));
        ESP_ERROR_CHECK(gpio_set_pull_mode(PIEZO_ATN10K_PIN, GPIO_PULLUP_ONLY));
        break;
    }
    ESP_ERROR_CHECK(gpio_set_level(PIEZO_G10_PIN, piezoParameters.gain));
}

unsigned long getPiezoSensorValue()
{
    return piezoSensorValue;
}

void resetPiezoSensorValue()
{
    piezoSensorValue = 0;
}
