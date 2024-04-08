#ifndef CAPACITIVESENSOR_H
#define CAPACITIVESENSOR_H

#define pulsePin 17
#define oscilatorPin 13


int initCapacitiveSensor();
void capacitiveSensorTask();
unsigned long getCapacitiveSensorValue();
void resetCapacitiveSensorValue();

_Bool getInBedStatus();
unsigned long getCapacityDiff();
unsigned long getCapacitythreshold();

void notifyMovement();


#endif