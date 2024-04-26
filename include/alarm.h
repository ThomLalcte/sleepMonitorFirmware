#ifndef ALARM_H
#define ALARM_H

#include "wakeupinator.h"

enum alarmState_t
{
    ALARM_OFF,
    ALARM_PRIMED,
    ALARM_ON,
    ALARM_MORE_ON,
    ALARM_FAILED,
    ALARM_PREWAKE,
};

void initAlarm();
void alarmTask();

int getAlarmState();
void setAlarmState(enum alarmState_t state);

#endif