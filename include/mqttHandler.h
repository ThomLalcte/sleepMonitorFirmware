#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

void initMQTT();
void sendMqttData(char* payload, char* topic);

#endif