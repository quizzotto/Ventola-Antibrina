#pragma once
#include <Arduino.h>
#include "esp_task_wdt.h"
#include "utilities.h"
#include "config.h"
#include "mqtt_handler.h"


// Definizioni per il modem
#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_MODEM_SIM7080

// Definizioni
extern TinyGsm modem;
extern TinyGsmClient client;
extern const char *register_info[];

enum {
    MODEM_CATM = 1,
    MODEM_NB_IOT,
    MODEM_CATM_NBIOT,
};

// Funzioni pubbliche
void setupModem();
void initModem();
void setupNetwork();
void checkConnessioni();
void checkSimStatus();

