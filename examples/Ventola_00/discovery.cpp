#include "discovery.h"

/***************************
* Discovery
****************************/
bool                auto_discovery = false;
const char*         model = "Ventola_NBIOT";                            // Hardware Model
const char*         swVersion = "1.0";                                  // Firmware Version
const char*         manufacturer = "s8s8";                              // Manufacturer Name
const char*         name = NOME;
const char*         suggested_area = "campo";

/***************************
* Topics MQTT
****************************/
JsonDocument doc;  // invece di StaticJsonDocument<1024> doc;
char TOPIC_TEMPERATURA[50];
char TOPIC_BATTERIA[50];
char TOPIC_STATO_VENTOLA[50];
char TOPIC_SETPOINT[50];
char TOPIC_ALLARME[50];
char TOPIC_TEMP_MIN[50];
char TOPIC_TEMPO_ACCESA[50];
char TOPIC_TEMPO_TOTALE[50];
char TOPIC_ERRORI[50];
char TOPIC_TEST[50];
char TOPIC_LAST_WILL[50];
char TOPIC_CMD[50];
char TOPIC_STATUS[50];


/***************************
* Discovery functions
****************************/
void initMQTTTopics() {
    snprintf(TOPIC_TEMPERATURA, 50, "%s%s/t/state", MQTT_GROUP, NOME);     
    snprintf(TOPIC_BATTERIA, 50, "%s%s/b/state", MQTT_GROUP, NOME);  
    snprintf(TOPIC_STATO_VENTOLA, 50, "%s%s/sv/state", MQTT_GROUP, NOME);
    snprintf(TOPIC_SETPOINT, 50, "%s%s/s/state", MQTT_GROUP, NOME);
    snprintf(TOPIC_ALLARME, 50, "%s%s/al/state", MQTT_GROUP, NOME);
    snprintf(TOPIC_TEMP_MIN, 50, "%s%s/tm/state", MQTT_GROUP, NOME);
    snprintf(TOPIC_TEMPO_ACCESA, 50, "%s%s/ta/state", MQTT_GROUP, NOME);
    snprintf(TOPIC_TEMPO_TOTALE, 50, "%s%s/tt/state", MQTT_GROUP, NOME);
    snprintf(TOPIC_ERRORI, 50, "%s%s/e/state", MQTT_GROUP, NOME);
    snprintf(TOPIC_LAST_WILL, 50, "%s%s/will", HA_PREFIX, NOME);
    snprintf(TOPIC_CMD, 50, "%s%s/cmd/state", MQTT_GROUP, NOME);
    snprintf(TOPIC_STATUS, 50, "%s%s/status", MQTT_GROUP, NOME);
}

void createDiscoveryTopic(const char* component, const char* subComponent, const char* name, const char* deviceClass, const char* icon, const char* unitOfMeasurement, const char* expireAfter) {
    #if DEBUG
        spl("Creazione topic di discovery per: " + String(name));
    #endif
    
    char topic[64];
    char uid[16];
    char state[64];
    JsonDocument doc;
    
    // Crea Topic per Discovery
    snprintf(topic, sizeof(topic), "%s%s/%s/%s/config", HA_PREFIX, component, NOME, subComponent);
    
    // Crea unique_id
    snprintf(uid, sizeof(uid), "%s_%s", NOME, subComponent);
    
    // Crea topic per pubblicare lo stato
    snprintf(state, sizeof(state), "%s%s/%s/state", MQTT_GROUP, NOME, subComponent);
    
    // Crea JSON payload per HA
    doc["name"] = name;
    doc["uniq_id"] = uid;
    doc["stat_t"] = state;
    doc["ic"] = icon;
    doc["exp_aft"] = expireAfter;

    if (strcmp(component, "sensor") == 0 || strcmp(component, "binary_sensor") == 0 || strcmp(component, "number") == 0) {
        doc["dev_cla"] = deviceClass;
        doc["unit_of_meas"] = unitOfMeasurement;
    }
    if (strcmp(component, "binary_sensor") == 0) {
        doc["pl_on"] = "on";
        doc["pl_off"] = "off";
    }
    if (strcmp(component, "number") == 0) {
        doc["min"] = "-5";
        doc["max"] = "5";
        doc["step"] = "0.1";
        doc["cmd_t"] = state;
        doc["mode"] = "slider";
    }
    if (strcmp(component, "switch") == 0) {
        doc["cmd_t"] = state;
    }
    if (strcmp(component, "text") == 0) {
        doc["cmd_t"] = state;
        doc["max"] = "50";
        doc["val_tpl"] = "{% set map = {'0': 'Tutto OK!','1': 'Errore Temperatura!','2': 'Batteria Bassa!','3': 'Errore Alternatore!'} %} {{ map[value] if value in map else 'error' }}"; 
    }

    // Aggiungi la configurazione per il select
    if (strcmp(component, "select") == 0) {
        doc["cmd_t"] = state;  // Topic per inviare il comando
        doc["stat_t"] = state; // Topic per lo stato
        
        // Definisci le opzioni disponibili
        JsonArray options = doc["options"].to<JsonArray>();
        options.add("Temperatura");
        options.add("DiscoveryOn");
        options.add("DiscoveryOff");
        options.add("InviaStato");
        options.add("Riavvia");
        
        // Aggiungi altre opzioni secondo necessità
    }

    // Configurazione del dispositivo principale
    JsonObject device = doc["device"].to<JsonObject>();
    device["ids"] = NOME;
    device["name"] = NOME;  // Usa NOME invece di name per il dispositivo
    device["mf"] = manufacturer;
    device["mdl"] = model;
    device["sw"] = swVersion; 
    device["sa"] = suggested_area;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    
    spl("Dimensione payload: " + String(jsonString.length()) + " bytes");
    
    
    if (publishMQTT(topic, jsonString.c_str(), true)) {
        spl("Discovery pubblicato per: " + String(name));
        spl("Topic: " + String(topic));
        spl("Payload: " + jsonString);
        
    } else {
        spl("Errore nella pubblicazione discovery per: " + String(name));
        spl("Topic: " + String(topic));
    }
}

void removeDiscoveryTopic(const char* component, const char* subComponent) {
    char topic[64];
    snprintf(topic, sizeof(topic), "%s%s/%s/%s/config", HA_PREFIX, component, NOME, subComponent);
    
    if (publishMQTT(topic, "", true)) {  // Pubblica stringa vuota con retained flag
        spl("Rimosso dispositivo: ");
        spl(subComponent);
    } else {
        spl("Errore nella rimozione del topic di discovery per: ");
        spl(subComponent);
    }
}

void haDiscovery() {
    if (auto_discovery) {
        spl("Aggiunta dei dispositivi per auto discovery");
        
        // Parte esistente per l'aggiunta dei dispositivi
        createDiscoveryTopic("sensor", "t", "Temperatura", "temperature", "mdi:thermometer", "°C", "660");
        delay(100);  // Aggiungi ritardo tra le pubblicazioni
        createDiscoveryTopic("sensor", "b", "Batteria", "voltage", "mdi:car-battery", "V", "660");
        delay(100);
        createDiscoveryTopic("binary_sensor", "sv", NOME , "moving", "mdi:wind-turbine", "", "660");
        delay(100);
        createDiscoveryTopic("number", "s", "Setpoint", "temperature", "mdi:thermometer-alert", "°C", "0");
        delay(100);
        createDiscoveryTopic("switch", "al", "Allarme", "", "mdi:alarm-bell", "", "0");
        delay(100);
        createDiscoveryTopic("sensor", "tm", "Temperatura Minima", "temperature", "mdi:thermometer-minus", "°C", "0");
        delay(100);
        createDiscoveryTopic("sensor", "ta", "Tempo Accensione", "duration", "mdi:chart-line", "m", "0");
        delay(100);
        createDiscoveryTopic("sensor", "tt", "Tempo Totale", "duration", "mdi:chart-line", "m", "0");
        delay(100);
        createDiscoveryTopic("text", "e", "Errori", "", "mdi:alert-circle", "", "0");
        delay(100);
        
        // Aggiungi il nuovo select per i comandi
        createDiscoveryTopic("select", "cmd", "Comandi", "", "mdi:console", "", "0");
        delay(100);
        
        spl("Dispositivo Aggiunto!");
        
        // Inizializza i valori
        delay(200);  // Attendi un po' più a lungo prima di inizializzare i valori
        publishMQTT(TOPIC_SETPOINT, "0", true);
        delay(50);
        publishMQTT(TOPIC_TEMPO_ACCESA, "0", true);
        delay(50);
        publishMQTT(TOPIC_TEMPO_TOTALE, "0", true);
        delay(50);
        publishMQTT(TOPIC_ERRORI, "0", true);
        delay(50);
        pubblicaStatoVentola(accesa);
        delay(50);
        pubblicaTemperatura(temperatura);
        delay(50);
        pubblicaVoltaggi(voltaggio_batteria);
    } else {
        // Parte per la rimozione dei dispositivi
        removeDiscoveryTopic("sensor", "t");
        removeDiscoveryTopic("sensor", "b");
        removeDiscoveryTopic("binary_sensor", "sv");
        removeDiscoveryTopic("number", "s");
        removeDiscoveryTopic("switch", "al");
        removeDiscoveryTopic("sensor", "tm");
        removeDiscoveryTopic("sensor", "ta");
        removeDiscoveryTopic("sensor", "tt");
        removeDiscoveryTopic("text", "e");
        removeDiscoveryTopic("select", "cmd");
        
        spl("Dispositivo Rimosso!");
    }
}

