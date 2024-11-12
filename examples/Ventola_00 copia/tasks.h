#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_handler.h"
#include "temperatura.h"
#include "funzioni.h"

// Definizione priorità task
#define PRIORITY_MQTT_MESSAGES    4  // Alta priorità
#define PRIORITY_TEMPERATURE     3
#define PRIORITY_MQTT_CONNECTION 2
#define PRIORITY_TIMER          2
#define PRIORITY_STATUS         1  // Bassa priorità

// Dimensioni stack per i task
#define STACK_SIZE_MQTT        4096
#define STACK_SIZE_TEMP        2048
#define STACK_SIZE_TIMER       2048
#define STACK_SIZE_STATUS      2048

// Handles per i task
extern TaskHandle_t mqttMessagesHandle;
extern TaskHandle_t temperatureHandle;
extern TaskHandle_t mqttConnectionHandle;
extern TaskHandle_t timerHandle;
extern TaskHandle_t statusHandle;

// Core assignment
#define CORE_MQTT      1  // Core comunicazioni
#define CORE_SENSORS   0  // Core sensori e logica

// Dichiarazione funzioni task
void startFreeRTOSTasks();
void stopFreeRTOSTasks(); 