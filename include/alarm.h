#ifndef ALARM_H
#define ALARM_H

#include "capactiveSensor.h"
#include "wakeupinator.h"

enum alarmState_t
{
    ALARM_OFF,
    ALARM_PRIMED,
    ALARM_ON,
    ALARM_FAILED,
    ALARM_PREWAKE,
};

void initAlarm();
void alarmTask();
void sampleCapacitiveDiff();

int getAlarmState();
void setAlarmState(enum alarmState_t state);
long getThreshold();
void setThreshold(long threshold);

#endif