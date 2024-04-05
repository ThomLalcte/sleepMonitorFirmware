#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

void initMQTT();
void sendMqttData(char* payload, char* topic);
void subscribeToTopic(char* topic, void (*callback)(char* payload, int payloadLength, char* topic));

#endif