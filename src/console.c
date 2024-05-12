#include "console.h"
#include "esp_log.h"
#include "esp_console.h"

#include "piezoSensor.h"
#include "capactiveSensor.h"
#include "presenceDetection.h"
#include "timeSync.h"
#include "alarm.h"

static const char *TAG = "console";


static int printCapacityDiff(int argc, char **argv)
{
    ESP_LOGI(TAG, "Last capacityDiff: %lu", getCapacityDiff());
    return 0;
}

esp_console_cmd_t capacityDiffCmd = {
    .command = "getCapacityDiff",
    .help = "Get the last capacity difference",
    .hint = NULL,
    .func = &printCapacityDiff,
};

static int printCapacityTotal(int argc, char **argv)
{
    ESP_LOGI(TAG, "Total capacity: %lu", getCapacitiveSensorValue());
    return 0;
}

esp_console_cmd_t capacityTotalCmd = {
    .command = "getCapacityTotal",
    .help = "Get the total capacity",
    .hint = NULL,
    .func = &printCapacityTotal,
};

static int printInBedStatus(int argc, char **argv)
{
    ESP_LOGI(TAG, "In bed status: %d", getInBedStatus());
    return 0;
}

esp_console_cmd_t inBedStatusCmd = {
    .command = "getInBedStatus",
    .help = "Get the in bed status",
    .hint = NULL,
    .func = &printInBedStatus,
};

static int printCapacityThreshold(int argc, char **argv)
{
    ESP_LOGI(TAG, "Capacity threshold: %lu", getCapacitythreshold());
    return 0;
}

esp_console_cmd_t capacityThresholdCmd = {
    .command = "getCapacityThreshold",
    .help = "Get the capacity threshold for in bed status",
    .hint = NULL,
    .func = &printCapacityThreshold,
};

static int printInbedCapacitance(int argc, char **argv)
{
    ESP_LOGI(TAG, "In bed capacitance: %lu", getInbedCapacitance());
    return 0;
}

esp_console_cmd_t inbedCapacitanceCmd = {
    .command = "getInbedCapacitance",
    .help = "Get the in bed capacitance",
    .hint = NULL,
    .func = &printInbedCapacitance,
};

static int printOutOfBedCapacitance(int argc, char **argv)
{
    ESP_LOGI(TAG, "Out of bed capacitance: %lu", getOutOfBedCapacitance());
    return 0;
}

esp_console_cmd_t outOfBedCapacitanceCmd = {
    .command = "getOutOfBedCapacitance",
    .help = "Get the out of bed capacitance",
    .hint = NULL,
    .func = &printOutOfBedCapacitance,
};

static int setInbedCapacitanceConsole(int argc, char **argv)
{
    if (argc < 2)
    {
        ESP_LOGI(TAG, "Usage: setInbedCapacitance <capacitance>");
        return 1;
    }
    // convert the string to a number
    unsigned long capacitance = strtoul(argv[1], NULL, 10);
    setInbedCapacitance(capacitance);
    return 0;
}

esp_console_cmd_t setInbedCapacitanceCmd = {
    .command = "setInbedCapacitance",
    .help = "Set the in bed capacitance",
    .hint = "<capacitance>",
    .func = &setInbedCapacitanceConsole,
};

static int setOutOfBedCapacitanceConsole(int argc, char **argv)
{
    if (argc < 2)
    {
        ESP_LOGI(TAG, "Usage: setOutOfBedCapacitance <capacitance>");
        return 1;
    }
    // convert the string to a number
    unsigned long capacitance = strtoul(argv[1], NULL, 10);
    setOutOfBedCapacitance(capacitance);
    return 0;
}

esp_console_cmd_t setOutOfBedCapacitanceCmd = {
    .command = "setOutOfBedCapacitance",
    .help = "Set the out of bed capacitance",
    .hint = "<capacitance>",
    .func = &setOutOfBedCapacitanceConsole,
};

static int printPiezoValue(int argc, char **argv)
{
    ESP_LOGI(TAG, "Piezo value: %lu", getPiezoSensorValue());
    return 0;
}

esp_console_cmd_t piezoValueCmd = {
    .command = "getPiezoValue",
    .help = "Get the piezo value",
    .hint = NULL,
    .func = &printPiezoValue,
};

static int printTime(int argc, char **argv)
{
    ESP_LOGI(TAG, "Time: %llu", getTime());
    return 0;
}

esp_console_cmd_t timeCmd = {
    .command = "getTime",
    .help = "Get the current time",
    .hint = NULL,
    .func = &printTime,
};

static int getNextAlarmCMD(int argc, char **argv)
{
    ESP_LOGI(TAG, "Next alarm: %lli", getNextAlarm());
    ESP_LOGI(TAG, "Next alarm strength: %d", getNextAlarmStrengh());
    return 0;
}

esp_console_cmd_t nextAlarmCmd = {
    .command = "getNextAlarm",
    .help = "Get the next alarm",
    .hint = NULL,
    .func = &getNextAlarmCMD,
};

void initConsole()
{
    ESP_LOGI(TAG, "Initializing console");

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

    repl_config.max_cmdline_length = 256;
    // enable history

    /* Register commands */
    esp_console_register_help_command();
    esp_console_cmd_register(&capacityDiffCmd);
    esp_console_cmd_register(&capacityTotalCmd);
    esp_console_cmd_register(&inBedStatusCmd);
    esp_console_cmd_register(&capacityThresholdCmd);
    esp_console_cmd_register(&inbedCapacitanceCmd);
    esp_console_cmd_register(&outOfBedCapacitanceCmd);
    esp_console_cmd_register(&setInbedCapacitanceCmd);
    esp_console_cmd_register(&setOutOfBedCapacitanceCmd);
    esp_console_cmd_register(&piezoValueCmd);
    esp_console_cmd_register(&timeCmd);
    esp_console_cmd_register(&nextAlarmCmd);


    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));


    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

void consoleTask()
{
    
}