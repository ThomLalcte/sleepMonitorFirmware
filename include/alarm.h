#ifndef ALARM_H
#define ALARM_H

#include "wakeupinator.h"
#include "stdint.h"

#define NEXT_ALARM_NVS_KEY "nextAlarm"
#define NEXT_ALARM_STRENGTH_NVS_KEY "nextAlarmStr"

enum alarmState_t
{
    ALARM_OFF,
    ALARM_PRIMED,
    ALARM_ON,
    ALARM_MORE_ON,
    ALARM_FAILED,
    ALARM_PRIMED_PREWAKE,
    ALARM_PREWAKE,
};

void initAlarm();
void alarmTask();

int getAlarmState();
void setAlarmState(enum alarmState_t state);

int64_t getNextAlarm();
unsigned char getNextAlarmStrengh();


#endif