#pragma once
#include <PubSubClient.h>
#include <TinyGsmClient.h>
#include "config.h"
#include "funzioni.h"

extern TinyGsmClient client;
extern PubSubClient mqttClient;

void setupMQTT();
void handleMQTTConnection();
void handleMQTTMessages();
bool publishMQTT(const char* topic, const char* payload, bool retain = false);
void onMqttMessage(char* topic, byte* payload, unsigned int length); 