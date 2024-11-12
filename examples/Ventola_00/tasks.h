#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_handler.h"
#include "temperatura.h"
#include "data_ora.h"
#include "utilities.h"
#include "modem.h"
#include "errori.h"
#include "voltaggi.h"
// Definizione priorità task
#define PRIORITY_MQTT_MESSAGES    4  // Alta priorità
#define PRIORITY_TEMPERATURE     3
#define PRIORITY_MQTT_CONNECTION 5  // Massima priorità per la connessione
#define PRIORITY_TIMER          2
#define PRIORITY_ERRORI         1  // Bassa priorità
#define PRIORITY_PUBLISH        3
#define PRIORITY_MODEM_CHECK    5  // Alta priorità come MQTT_CONNECTION
#define PRIORITY_VOLTAGGI       3

// Dimensioni stack per i task
#define STACK_SIZE_MQTT        4096
#define STACK_SIZE_TEMP        2048
#define STACK_SIZE_MQTT_CONNECTION 8192
#define STACK_SIZE_TIMER       2048
#define STACK_SIZE_ERRORI      2048
#define STACK_SIZE_PUBLISH      2048
#define STACK_SIZE_MODEM_CHECK  2048
#define STACK_SIZE_VOLTAGGI    2048

// Handles per i task
extern TaskHandle_t mqttMessagesHandle;
#if ENABLE_TASK_TEMPERATURA
extern TaskHandle_t temperatureHandle;
#endif
extern TaskHandle_t mqttConnectionHandle;
extern TaskHandle_t timerHandle;
#if ENABLE_TASK_ERRORI
extern TaskHandle_t statusErroriHandle;
#endif
#if ENABLE_TASK_PUBLISH
extern TaskHandle_t publishTaskHandle;
#endif
extern TaskHandle_t modemCheckHandle;   
#if ENABLE_TASK_VOLTAGGI
extern TaskHandle_t voltaggiHandle;
#endif

// Core assignment
#define CORE_MQTT      1  // Core comunicazioni
#define CORE_SENSORS   0  // Core sensori e logica

// Dichiarazione funzioni task
void startFreeRTOSTasks();
void stopFreeRTOSTasks(); 