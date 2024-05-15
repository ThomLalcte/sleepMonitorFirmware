#ifndef WAKEY_H
#define WAKEY_H

#define motorPin 15
#define buzzPinA 13
#define buzzPinB 14
#define ledPin 2
#define cycleTime 1000
#define motorWatchdog 300 // in seconds


void initWakeupinator();
void enableWakeupinator();
void disableWakeupinator();
void wakeupinatorTask();
void setVibecheckLevel(const unsigned short level);

#endif