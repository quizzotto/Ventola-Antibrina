#pragma once
#include <Arduino.h>

/***************************
* Nome Ventola
****************************/
#define NOME "V_00"
#define batteria 24

/***************************
* Parametri Brinate Default
****************************/
#define DEFAULT_GIORNO_INIZIO_BRINATE  61    // 1 marzo
#define DEFAULT_GIORNO_FINE_BRINATE    122   // 1 maggio

// Variabili modificabili
//extern int giorno_inizio_brinate;
//extern int giorno_fine_brinate;

/***************************
* DEBUG
****************************/
#define DEBUG 1

#if DEBUG
#define sp(x) Serial.print(x);
#define spl(x) Serial.println(x);
#else 
#define sp(x)
#define spl(x)
#endif

/***************************
* MODEM 
****************************/
#define APN "iot.1nce.net"
#define CONNECTION_RETRY_DELAY    10000

/***************************
* PIN Configuration
****************************/
#define ONE_WIRE_BUS 10    // Sonda temperatura (GPIO10)
#define RELAY_PIN    11    // Relè (GPIO11)
#define BUTTON_PIN   12   // Pulsante (GPIO12)

// Configurazione pin
#define RELAY_ON     HIGH  // o LOW in base al tuo relè
#define RELAY_OFF    LOW   // o HIGH in base al tuo relè

/***************************
* MQTT Configuration
****************************/
#define MQTT_SERVER          "128.116.210.227"
#define MQTT_PORT           1883
#define MQTT_USERNAME       "ventole"
#define MQTT_PASSWORD       ""
#define MQTT_CLIENT_ID      NOME
#define MQTT_GROUP          "ventole/"
#define HA_PREFIX          "homeassistant/"

// Topic structure
#define MQTT_BASE_TOPIC "ventole/" NOME
#define MQTT_STATUS_TOPIC MQTT_BASE_TOPIC "/status"
#define MQTT_TELEMETRY_TOPIC MQTT_BASE_TOPIC "/telemetry"
#define MQTT_COMMAND_TOPIC MQTT_BASE_TOPIC "/cmd"

// Aumenta il buffer MQTT
#define MQTT_MAX_PACKET_SIZE 1024  // o anche 1024 se necessario

// Temperature
#define TEMP_CHECK_INTERVAL 10000  // 10 secondi
#define TEMP_CONVERSION_TIME 1000   // tempo di conversione in ms

