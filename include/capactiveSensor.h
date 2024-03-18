#ifndef CAPACITIVESENSOR_H
#define CAPACITIVESENSOR_H

#define pulsePin 17
#define oscilatorPin 13

#define TAG "CapacitiveSensor"

int initCapacitiveSensor();
void capacitiveSensorTask();
unsigned long getCapacitiveSensorValue();

#endif