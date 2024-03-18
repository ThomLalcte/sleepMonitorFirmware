#ifndef WAKEY_H
#define WAKEY_H

#define motorPin 15
#define cycleTime 1000


void initWakeupinator();
void enableWakeupinator();
void disableWakeupinator();
void wakeupinatorTask();

#endif