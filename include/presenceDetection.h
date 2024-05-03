#ifndef PRESENCEDETECTION_H
#define PRESENCEDETECTION_H

#include <stdbool.h>

void initPresenceDetection();
void presenceDetectionTask();
void notifyMovement();

unsigned long getCapacitythreshold();
unsigned long getInbedCapacitance();
unsigned long getOutOfBedCapacitance();
void setInbedCapacitance(unsigned long capacitance);
void setOutOfBedCapacitance(unsigned long capacitance);
_Bool getInBedStatus();

#endif