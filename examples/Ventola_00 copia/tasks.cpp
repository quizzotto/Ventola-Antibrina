#include "tasks.h"

// Handles dei task
TaskHandle_t mqttMessagesHandle = NULL;
TaskHandle_t temperatureHandle = NULL;
TaskHandle_t mqttConnectionHandle = NULL;
TaskHandle_t timerHandle = NULL;
TaskHandle_t statusHandle = NULL;

// Task MQTT Messages
void mqttMessagesTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(100);
    while(1) {
        if (mqttClient.connected()) {
            mqttClient.loop();
        }
        vTaskDelay(xDelay);
    }
}

// Task Temperatura
void temperatureTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(TEMP_CHECK_INTERVAL);
    
    while(1) {
        readTemperature();
        vTaskDelay(xDelay);
    }
}

// Task MQTT Connection
void mqttConnectionTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(CONNECTION_RETRY_DELAY);
    while(1) {
        checkConnessioni();
        handleMQTTConnection();
        vTaskDelay(xDelay);
    }
}

// Task Timer
void timerTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(1000); // 1 secondo
    while(1) {
        checkAndUpdateTimer();
        vTaskDelay(xDelay);
    }
}

// Task Status (ventola, errori, etc)
void statusTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(10000); // 10 secondi
    while(1) {
        statoVentola();
        checkErrors();
        vTaskDelay(xDelay);
    }
}

void startFreeRTOSTasks() {
    // Core MQTT (1)
    xTaskCreatePinnedToCore(
        mqttMessagesTask,
        "MQTT_Messages",
        STACK_SIZE_MQTT,
        NULL,
        PRIORITY_MQTT_MESSAGES,
        &mqttMessagesHandle,
        CORE_MQTT);
        
    xTaskCreatePinnedToCore(
        mqttConnectionTask,
        "MQTT_Connection",
        STACK_SIZE_MQTT,
        NULL,
        PRIORITY_MQTT_CONNECTION,
        &mqttConnectionHandle,
        CORE_MQTT);

    // Core Sensori (0)
    xTaskCreatePinnedToCore(
        temperatureTask,
        "Temperature",
        STACK_SIZE_TEMP,
        NULL,
        PRIORITY_TEMPERATURE,
        &temperatureHandle,
        CORE_SENSORS);
        
    xTaskCreatePinnedToCore(
        timerTask,
        "Timer",
        STACK_SIZE_TIMER,
        NULL,
        PRIORITY_TIMER,
        &timerHandle,
        CORE_SENSORS);
        
    xTaskCreatePinnedToCore(
        statusTask,
        "Status",
        STACK_SIZE_STATUS,
        NULL,
        PRIORITY_STATUS,
        &statusHandle,
        CORE_SENSORS);
}

void stopFreeRTOSTasks() {
    if (mqttMessagesHandle) vTaskDelete(mqttMessagesHandle);
    if (temperatureHandle) vTaskDelete(temperatureHandle);
    if (mqttConnectionHandle) vTaskDelete(mqttConnectionHandle);
    if (timerHandle) vTaskDelete(timerHandle);
    if (statusHandle) vTaskDelete(statusHandle);
} 