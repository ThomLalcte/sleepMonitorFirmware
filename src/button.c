#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "button"

// define the callback function
void (*buttonActionCallback)(void) = NULL;

char runButtonAction = 0;

static void gpio_isr_handler(void *arg)
{
    runButtonAction = 1;
}

void initButtonGPIOs()
{
    // setup the pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << buttonPin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLDOWN_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(buttonPin, gpio_isr_handler, (void *)buttonPin));
}

void initButton()
{
    ESP_LOGI(TAG, "Initializing button");

    initButtonGPIOs();
}

void buttonTask()
{
    if (runButtonAction)
    {
        runButtonAction = 0;
        ESP_LOGI(TAG, "Button pressed: running callback");
        if (buttonActionCallback != NULL)
            (*buttonActionCallback)();
    }
}

void setButtonPressedCallback(void (*callback)())
{
    buttonActionCallback = callback;
}
