#ifndef ALARM_H
#define ALARM_H

#include "capactiveSensor.h"
#include "wakeupinator.h"

enum alarmState_t
{
    ALARM_OFF,
    ALARM_PRIMED,
    ALARM_ON,
    ALARM_FAILED
};

void initAlarm();
void alarmTask();
void sampleCapacitiveDiff();


void setAlarmState(enum alarmState_t state);
void setThreshold(unsigned long threshold);

#endif