#ifndef CAPACITIVESENSOR_H
#define CAPACITIVESENSOR_H

#define pulsePin 17
#define oscilatorPin 13

static unsigned long capacityDiffThreshold = 60000000/3/60; // debug value #TODO: place it back in the .c file

int initCapacitiveSensor();
void capacitiveSensorTask();
unsigned long getCapacitiveSensorValue();
void resetCapacitiveSensorValue();

_Bool getTresholdCrossed();
unsigned long getCapacityDiff();

#endif