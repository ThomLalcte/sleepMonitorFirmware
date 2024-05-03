#include "presenceDetection.h"

#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "capactiveSensor.h"
#include "piezoSensor.h"
#include "secrets.h"
#include "mqttHandler.h"

#define TAG "PresenceDetection"

// calibration definitions
int64_t lastMovement = (int64_t)6.48e7;
int64_t lastCalibration = 0;
unsigned long highCapacity = 275000;
unsigned long lowCapacity = 200000;
unsigned long capacityDiffThreshold = 0;
unsigned long capacityDiffHysteresis = 0;
_Bool inBed = false;
int presenceUpdateCounter = 0;

void saveCalibrationValues()
{
  ESP_LOGI(TAG, "Saving calibration values: highCapacity: %lu, lowCapacity: %lu", highCapacity, lowCapacity);

  nvs_handle_t nvsHandle;
  ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));
  ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "highCapacity", highCapacity));
  ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "lowCapacity", lowCapacity));
  ESP_ERROR_CHECK(nvs_commit(nvsHandle));
  nvs_close(nvsHandle);
}

void computeThresholds()
{
  capacityDiffHysteresis = (highCapacity - lowCapacity) / 4;
  capacityDiffThreshold = (highCapacity + lowCapacity + capacityDiffHysteresis) / 2;
}

unsigned long getInbedCapacitance()
{
  return lowCapacity;
}

unsigned long getOutOfBedCapacitance()
{
  return highCapacity;
}

void setInbedCapacitance(unsigned long capacitance)
{
  lowCapacity = capacitance;
  computeThresholds();
  saveCalibrationValues();
}

void setOutOfBedCapacitance(unsigned long capacitance)
{
  highCapacity = capacitance;
  computeThresholds();
  saveCalibrationValues();
}

void updateCalibration()
{
  int64_t time_us = esp_timer_get_time();
  if (time_us - lastMovement > 1000000ULL * 60ULL * 60ULL * 2ULL)
  {
    if (!getInBedStatus() && time_us - lastCalibration > 1000000ULL * 60ULL * 60ULL)
    {
      highCapacity = (highCapacity * 3 + getCapacityDiff()) / 4;
      if (highCapacity > 500000)
      {
        highCapacity = 500000;
      }
      computeThresholds();
      lastCalibration = time_us;
    }
  }
  else if (time_us - lastMovement > 10000000)
  {
    if (getInBedStatus() && time_us - lastCalibration > 1000000ULL * 60ULL)
    {
      lowCapacity = (lowCapacity * 3 + getCapacityDiff()) / 4;
      if (lowCapacity < 100000)
      {
        lowCapacity = 100000;
      }
      computeThresholds();
      lastCalibration = time_us;
    }
  }
}

void notifyMovement()
{
  ESP_LOGI(TAG, "Movement detected");
  lastMovement = esp_timer_get_time();
}

void updateInBedStatus()
{
  if (presenceUpdateCounter < 3)
  {
    presenceUpdateCounter++;
    return;
  }
  if (getPiezoSensorValue() < 30)
    return;
  if (inBed)
  {
    if (getCapacityDiff() > capacityDiffThreshold + capacityDiffHysteresis)
    {
      ESP_LOGI(TAG, "Out of bed");
      char payload[100];
      sprintf(payload, "{\"inBed\": false, \"capacity\":%lu}", getCapacityDiff());
      sendMqttData(payload, MQTT_TOPIC_WAKEUP, 1);
      inBed = false;
      saveCalibrationValues();
    }
  }
  else
  {
    if (getCapacityDiff() < capacityDiffThreshold - capacityDiffHysteresis)
    {
      ESP_LOGI(TAG, "In bed");
      char payload[100];
      sprintf(payload, "{\"inBed\": true, \"capacity\":%lu}", getCapacityDiff());
      sendMqttData(payload, MQTT_TOPIC_WAKEUP, 1);
      inBed = true;
    }
  }
}

_Bool getInBedStatus()
{
  return inBed;
}

unsigned long getCapacitythreshold()
{
  return capacityDiffThreshold;
}

void initPresenceDetection()
{
  ESP_LOGI(TAG, "Starting presence detection");

  // read the threshold values from the NVS
  nvs_handle_t nvsHandle;
  ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));
  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u32(nvsHandle, "highCapacity", &highCapacity));
  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u32(nvsHandle, "lowCapacity", &lowCapacity));
  nvs_close(nvsHandle);

  computeThresholds();

  ESP_LOGI(TAG, "highCapacity: %lu, lowCapacity: %lu, capacityDiffThreshold: %lu, capacityDiffHysteresis: %lu", highCapacity, lowCapacity, capacityDiffThreshold, capacityDiffHysteresis);

  ESP_LOGI(TAG, "Presence detection done initializing");
}

void presenceDetectionTask()
{
  updateCalibration();
  updateInBedStatus();
}