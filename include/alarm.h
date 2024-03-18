#ifndef ALARM_H
#define ALARM_H

#include "capactiveSensor.h"
#include "wakeupinator.h"

#define TAG "alarm"

enum alarmState_t
{
    ALARM_OFF,
    ALARM_PRIMED,
    ALARM_ON,
    ALARM_FAILED
};

void alarmInit();
void alarmTask();
unsigned long sampleCapacitiveDiff();


void setAlarmState(enum alarmState_t state);
void setThreshold(unsigned long threshold);

#endif