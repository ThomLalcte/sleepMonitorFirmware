#ifndef CAPACITIVESENSOR_H
#define CAPACITIVESENSOR_H

#include "stdint.h"

#define pulsePin 17
#define oscilatorPin 13
#define capacityDiffMeanFactor 3

int initCapacitiveSensor();
void capacitiveSensorTask();
void stopCapacitiveSensor();
void startCapacitiveSensor();
void resetCapacitiveSensorValue();

int32_t getCapacityDiff();
int32_t getCapacitiveSensorValue();
int32_t getCapacityDiffMean();

#endif