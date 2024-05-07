#ifndef CAPACITIVESENSOR_H
#define CAPACITIVESENSOR_H

#define pulsePin 17
#define oscilatorPin 13
#define capacityDiffMeanFactor 10.0

int initCapacitiveSensor();
void capacitiveSensorTask();
void stopCapacitiveSensor();
void startCapacitiveSensor();
void resetCapacitiveSensorValue();

unsigned long getCapacityDiff();
unsigned long getCapacitiveSensorValue();
unsigned long getCapacityDiffMean();

#endif