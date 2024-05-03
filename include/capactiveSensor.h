#ifndef CAPACITIVESENSOR_H
#define CAPACITIVESENSOR_H

#define pulsePin 17
#define oscilatorPin 13


int initCapacitiveSensor();
void capacitiveSensorTask();
unsigned long getCapacitiveSensorValue();
void resetCapacitiveSensorValue();

unsigned long getCapacityDiff();

#endif