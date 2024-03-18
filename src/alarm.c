#include "alarm.h"
#include "stdbool.h"
#include "driver/gptimer.h"

enum alarmState_t alarmState = ALARM_OFF;

unsigned long capactityDiff = 0;
unsigned long initCapactityDiff = 0;
unsigned long lastCapacity = 0;
unsigned long outThreshold = 0;
bool checkCapacitiveSensorFlag = false;
unsigned int timerCounter = 0;

void setAlarmState(enum alarmState_t state)
{
    alarmState = state;
}

void setThreshold(unsigned long threshold)
{
    outThreshold = threshold;
}

void timerCallback(void *arg)
{
    checkCapacitiveSensorFlag = true;
    timerCounter++;
}

void initAlarmTimer()
{
    // Initialize the alarm timer
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
}

void alarmInit()
{
    initAlarmTimer();
}

void alarmTask()
{

    if (checkCapacitiveSensorFlag && (alarmState == ALARM_PRIMED || alarmState == ALARM_ON))
    {
        sampleCapacitiveDiff();
        checkCapacitiveSensorFlag = false;
    }

    switch (alarmState)
    {
    case ALARM_OFF:
        break;
    case ALARM_PRIMED:
        if (checkCapacitiveSensorFlag)
        {
            initCapactityDiff = capactityDiff;
            alarmState = ALARM_ON;
            enableWakeupinator();
        }
        break;
    case ALARM_ON:
        if (capactityDiff > outThreshold)
        {
            disableWakeupinator();
        }
        break;
    case ALARM_FAILED:
        break;
    }
}

unsigned long sampleCapacitiveDiff()
{
    unsigned long currentCapacity = getCapacitiveSensorValue();
    capactityDiff = lastCapacity - currentCapacity;
    lastCapacity = currentCapacity;
    return capactityDiff;
}