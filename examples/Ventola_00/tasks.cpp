#include "tasks.h"

// Handles dei task
TaskHandle_t mqttMessagesHandle = NULL;
TaskHandle_t temperatureHandle = NULL;
TaskHandle_t mqttConnectionHandle = NULL;
TaskHandle_t timerHandle = NULL;
TaskHandle_t statusErroriHandle = NULL;
TaskHandle_t publishTaskHandle = NULL;
TaskHandle_t modemCheckHandle = NULL;
TaskHandle_t voltaggiHandle = NULL;

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
    const TickType_t xDelay = pdMS_TO_TICKS(5000); // 5 secondi tra i tentativi
    while(1) {
        handleMQTTConnection();
        vTaskDelay(xDelay);
    }
}

// Task Timer
void timerTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(60000); // 1 secondo
    while(1) {
        aggiornaDataOra();
        vTaskDelay(xDelay);
    }
}

// Task Status (ventola, errori, etc)
void statusErroriTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(2000);
    while(1) {
        pubblicaStatoErrore();
        vTaskDelay(xDelay);
    }
}

// Task Pubblicazione (Temperature e altri stati)
void publishTask(void *pvParameters) {
    while(1) {
        // Pubblica le temperature
        pubblicaTemperatura(temperatura);
        pubblicaTemperaturaMinima(temperatura_min);
        
        // Pubblica i voltaggi e stati
        pubblicaVoltaggi(voltaggio_batteria);
        pubblicaStatoVentola(accesa);
        
        vTaskDelay(pdMS_TO_TICKS(intervalloPubTemp));
    }
}

// Task controllo connessione modem
void modemCheckTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(30000); // Controlla ogni 30 secondi
    
    while(1) {
        checkConnessioni();
        vTaskDelay(xDelay);
    }
}
// Task Voltaggi
void voltaggiTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(VOLTAGE_CHECK_INTERVAL);
    while(1) {
        readVoltaggi();
        vTaskDelay(xDelay);
    }
}

// Task per monitorare l'utilizzo degli stack
void stackMonitorTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(30000);  // Controlla ogni 30 secondi
    
    while(1) {
        spl("\n=== Stack Usage Report ===");
        
        // Monitora ogni task
        if (mqttMessagesHandle) {
            spl("MQTT Messages Task: " + String(uxTaskGetStackHighWaterMark(mqttMessagesHandle)) + " words free");
        }
        if (temperatureHandle) {
            spl("Temperature Task: " + String(uxTaskGetStackHighWaterMark(temperatureHandle)) + " words free");
        }
        if (mqttConnectionHandle) {
            spl("MQTT Connection Task: " + String(uxTaskGetStackHighWaterMark(mqttConnectionHandle)) + " words free");
        }
        if (timerHandle) {
            spl("Timer Task: " + String(uxTaskGetStackHighWaterMark(timerHandle)) + " words free");
        }
        if (statusErroriHandle) {
            spl("Status Task: " + String(uxTaskGetStackHighWaterMark(statusErroriHandle)) + " words free");
        }
        if (publishTaskHandle) {
            spl("Publish Task: " + String(uxTaskGetStackHighWaterMark(publishTaskHandle)) + " words free");
        }
        if (modemCheckHandle) {
            spl("Modem Check Task: " + String(uxTaskGetStackHighWaterMark(modemCheckHandle)) + " words free");
        }
        if (voltaggiHandle) {
            spl("Voltaggi Task: " + String(uxTaskGetStackHighWaterMark(voltaggiHandle)) + " words free");
        }
        
        spl("========================\n");
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
    delay(100);
    xTaskCreatePinnedToCore(
        mqttConnectionTask,
        "MQTT_Connection",
        STACK_SIZE_MQTT,
        NULL,
        PRIORITY_MQTT_CONNECTION,
        &mqttConnectionHandle,
        CORE_MQTT);
    delay(100);
    // Core Sensori (0)
    #if ENABLE_TASK_TEMPERATURA
    xTaskCreatePinnedToCore(
        temperatureTask,
        "Temperature",
        STACK_SIZE_TEMP,
        NULL,
        PRIORITY_TEMPERATURE,
        &temperatureHandle,
        CORE_SENSORS);
    delay(100);
    #endif
    xTaskCreatePinnedToCore(
        timerTask,
        "Timer",
        STACK_SIZE_TIMER,
        NULL,
        PRIORITY_TIMER,
        &timerHandle,
        CORE_SENSORS);
    delay(100);
    #if ENABLE_TASK_ERRORI
    xTaskCreatePinnedToCore(
        statusErroriTask,
        "Status",
        STACK_SIZE_ERRORI,
        NULL,
        PRIORITY_ERRORI,
        &statusErroriHandle,
        CORE_SENSORS);
    delay(100);
    #endif
    #if ENABLE_TASK_PUBLISH
    xTaskCreatePinnedToCore(
        publishTask,
        "Publish_Task",
        STACK_SIZE_PUBLISH,
        NULL,
        PRIORITY_PUBLISH,
        &publishTaskHandle,
        CORE_SENSORS);
    delay(100);
    #endif
    xTaskCreatePinnedToCore(
        modemCheckTask,
        "Modem_Check",
        STACK_SIZE_MODEM_CHECK,
        NULL,
        PRIORITY_MODEM_CHECK,
        &modemCheckHandle,
        CORE_MQTT);
    delay(100);
    xTaskCreatePinnedToCore(
        stackMonitorTask,
        "Stack_Monitor",
        2048,
        NULL,
        1,  // Bassa priorità
        NULL,  // Non serve salvare l'handle
        CORE_SENSORS
    );
    delay(100);
    #if ENABLE_TASK_VOLTAGGI
    xTaskCreatePinnedToCore(
        voltaggiTask,
        "Voltaggi",
        STACK_SIZE_VOLTAGGI,
        NULL,
        PRIORITY_VOLTAGGI,
        &voltaggiHandle,
        CORE_SENSORS);
    delay(100);
    #endif
}

void stopFreeRTOSTasks() {
    if (mqttMessagesHandle) vTaskDelete(mqttMessagesHandle);
    if (temperatureHandle) vTaskDelete(temperatureHandle);
    if (mqttConnectionHandle) vTaskDelete(mqttConnectionHandle);
    if (timerHandle) vTaskDelete(timerHandle);
    if (statusErroriHandle) vTaskDelete(statusErroriHandle);
    if (publishTaskHandle) vTaskDelete(publishTaskHandle);
    if (modemCheckHandle) vTaskDelete(modemCheckHandle);
    if (voltaggiHandle) vTaskDelete(voltaggiHandle);
} 