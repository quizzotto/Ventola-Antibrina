#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "mqtt_handler.h"
#include "voltaggi.h"

/***************************
* Discovery
****************************/
extern bool auto_discovery;
void initMQTTTopics();
void haDiscovery();

/***************************
* Topics MQTT
****************************/
extern char TOPIC_TEMPERATURA[50];
extern char TOPIC_TEMP_MIN[50];
extern char TOPIC_CMD[50];
extern char TOPIC_LAST_WILL[50];
extern char TOPIC_STATO_VENTOLA[50];
extern char TOPIC_TEMPO_ACCESA[50];
extern char TOPIC_TEMPO_TOTALE[50];
extern char TOPIC_ERRORI[50];
extern char TOPIC_BATTERIA[50];
extern char TOPIC_STATUS[50];

