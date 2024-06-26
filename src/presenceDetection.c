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
int64_t lastPresenceUpdate = 0;
int64_t lastCalibration = 0;
int64_t gotInBed = 0;
int64_t leftBed = 0;
unsigned long highCapacity = 275000;
unsigned long lowCapacity = 200000;
unsigned long capacityDiffThreshold = 0;
unsigned long capacityDiffHysteresis = 0;
_Bool inBed = false;
_Bool movementFlag = false;

void saveCalibrationValues()
{
  ESP_LOGI(TAG, "Saving calibration values: highCapacity: %lu, lowCapacity: %lu, capacityDiffThreshold: %lu, capacityDiffHysteresis: %lu", highCapacity, lowCapacity, capacityDiffThreshold, capacityDiffHysteresis);

  nvs_handle_t nvsHandle;
  ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));
  ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, HIGH_THRESHOLD_NVS_KEY, highCapacity));
  ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, LOW_THRESHOLD_NVS_KEY, lowCapacity));
  ESP_ERROR_CHECK(nvs_commit(nvsHandle));
  nvs_close(nvsHandle);
}

void computeThresholds()
{
  capacityDiffHysteresis = (highCapacity - lowCapacity) >> 2;
  capacityDiffThreshold = (highCapacity + lowCapacity + capacityDiffHysteresis) >> 1;

  if (capacityDiffThreshold > highCapacity)
  {
    ESP_LOGE(TAG, "Threshold is higher than highCapacity: %lu > %lu", capacityDiffThreshold, highCapacity);
  }
  if (capacityDiffThreshold < lowCapacity)
  {
    ESP_LOGE(TAG, "Threshold is lower than lowCapacity: %lu < %lu", capacityDiffThreshold, lowCapacity);
  }
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

  if ((time_us - lastCalibration) > 1000000ULL * 60ULL * 60ULL * 12LL)
  {
    if (getInBedStatus())
    {
      if ((time_us - gotInBed) > 1000000ULL * 60ULL * 60ULL * 2LL)
      {
        highCapacity = (highCapacity * 3 + getCapacityDiffMean()) >> 2;
        if (highCapacity > 500000)
        {
          ESP_LOGW(TAG, "High capacity reached limit: %lu (set to 500000)", highCapacity);
          highCapacity = 500000;
        }
        else if (highCapacity < lowCapacity)
        {
          ESP_LOGE(TAG, "High capacity is lower than low capacity: %lu < %lu", highCapacity, lowCapacity);
        }
        computeThresholds();
        lastCalibration = time_us;
      }
    }
    else
    {
      if ((time_us - leftBed) > 1000000ULL * 60ULL * 60ULL * 2LL)
      {
        lowCapacity = (lowCapacity * 3 + getCapacityDiffMean()) >> 2;
        if (lowCapacity < 100000)
        {
          ESP_LOGW(TAG, "Low capacity reached limit: %lu (set to 100000)", lowCapacity);
          lowCapacity = 100000;
        }
        else if (lowCapacity > highCapacity)
        {
          ESP_LOGE(TAG, "Low capacity is higher than high capacity: %lu > %lu", lowCapacity, highCapacity);
        }
        computeThresholds();
        lastCalibration = time_us;
      }
    }
  }
  // if ((time_us - lastMovement) > 1000000ULL * 60ULL * 60ULL * 2ULL)
  // {
  //   if (!getInBedStatus() && (time_us - lastCalibration) > 1000000ULL * 60ULL * 60ULL * 12LL)
  //   {
  //     highCapacity = (highCapacity * 3 + getCapacityDiffMean()) >> 2;
  //     if (highCapacity > 500000)
  //     {
  //       highCapacity = 500000;
  //     }
  //     else if (highCapacity < lowCapacity)
  //     {
  //       ESP_LOGE(TAG, "High capacity is lower than low capacity: %lu < %lu", highCapacity, lowCapacity);
  //     }
  //     computeThresholds();
  //     lastCalibration = time_us;
  //   }
  // }
  // else if ((time_us - lastMovement) > 10000000)
  // {
  //   if (getInBedStatus() && (time_us - lastCalibration) > 1000000ULL * 60ULL * 60ULL * 12LL)
  //   {
  //     lowCapacity = (lowCapacity * 3 + getCapacityDiffMean()) >> 2;
  //     if (lowCapacity < 100000)
  //     {
  //       lowCapacity = 100000;
  //     }
  //     else if (lowCapacity > highCapacity)
  //     {
  //       ESP_LOGE(TAG, "Low capacity is higher than high capacity: %lu > %lu", lowCapacity, highCapacity);
  //     }
  //     computeThresholds();
  //     lastCalibration = time_us;
  //   }
  // }
}

void notifyMovement()
{
  // ESP_LOGI(TAG, "Movement detected");
  lastMovement = esp_timer_get_time();
  movementFlag = true;
}

void updateInBedStatus()
{
  int64_t time_us = esp_timer_get_time();
  /*if (movementFlag && (time_us - lastMovement) > 1000000ULL * 5ULL)
  {
    ESP_LOGD(TAG, "Piezo detected movement: %lu\nUpdating inBed status", getPiezoSensorValue());
    movementFlag = false;
  }
  else */if (time_us - lastPresenceUpdate < 1000000ULL * 5ULL)
  {
    return;
  }

  int32_t diff = getCapacityDiff();
  lastPresenceUpdate = time_us;

  if (inBed)
  {
    if (diff > capacityDiffThreshold + capacityDiffHysteresis)
    {
      ESP_LOGI(TAG, "Out of bed (%lu)", diff);
      char payload[100];
      sprintf(payload, "{\"inBed\": false, \"capacity\":%lu}", diff);
      sendMqttData(payload, MQTT_TOPIC_WAKEUP, 1);
      inBed = false;
      leftBed = time_us;
    }
  }
  else
  {
    if (diff < capacityDiffThreshold - capacityDiffHysteresis && diff != 0)
    {
      ESP_LOGI(TAG, "In bed (%lu)", diff);
      char payload[100];
      sprintf(payload, "{\"inBed\": true, \"capacity\":%lu}", diff);
      sendMqttData(payload, MQTT_TOPIC_WAKEUP, 1);
      inBed = true;
      gotInBed = time_us;
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
  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u32(nvsHandle, HIGH_THRESHOLD_NVS_KEY, &highCapacity));
  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u32(nvsHandle, LOW_THRESHOLD_NVS_KEY, &lowCapacity));
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