#ifndef PIEZO_H
#define PIEZO_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIEZO_G10_PIN 32
#define PIEZO_ATN1MEG_PIN 33
#define PIEZO_ATN100K_PIN 34
#define PIEZO_ATN10K_PIN 35
#define piezoSamplingFrequency SOC_ADC_SAMPLE_FREQ_THRES_LOW
#define convFrameSize SOC_ADC_DIGI_DATA_BYTES_PER_CONV * (SOC_ADC_SAMPLE_FREQ_THRES_LOW/300)
#define piezoSampleBufferSize convFrameSize*4


#define TAG "piezoSensor"

void initPiezoSensor(TaskHandle_t* mainTaskHandle);
void piezoSensorTask();
void increasePiezoSensitivity();
void decreasePiezoSensitivity();
void setPiezoSensitivity(short attenuation, unsigned short gain);

void readPiezoSensor();

#endif