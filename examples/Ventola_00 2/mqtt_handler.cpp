#include "mqtt_handler.h"
#include <ArduinoJson.h>

// Configurazione MQTT
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;

// Dichiarazione del client MQTT
PubSubClient mqttClient(client);

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    static JsonDocument doc;
    
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        spl("Errore parsing JSON: " + String(error.c_str()));
        return;
    }
    
    spl("Messaggio MQTT ricevuto [" + String(topic) + "]: " + String(message));

    // Gestione comando discovery
    if (doc["discovery"].is<JsonVariant>()) {
        bool newDiscovery;
        if (doc["discovery"].is<bool>()) {
            newDiscovery = doc["discovery"].as<bool>();
        } else {
            String discoveryStr = doc["discovery"].as<const char*>();
            newDiscovery = (discoveryStr == "true" || discoveryStr == "1");
        }
        
        auto_discovery = newDiscovery;
        spl("Auto discovery " + String(auto_discovery ? "abilitato" : "disabilitato"));
        haDiscovery();
        
        String response = "{\"device\":\"" + String(NOME) + 
                         "\",\"discovery\":" + String(auto_discovery ? "true" : "false") + "}";
        publishMQTT(MQTT_STATUS_TOPIC, response.c_str(), true);
        return;
    }

    // Gestione comando temperatura
    if (doc["command"].is<const char*>() && strcmp(doc["command"].as<const char*>(), "temperatura") == 0) {
        #if DEBUG
            spl("Richiesta temperatura ricevuta");
        #endif
        
        String response = "{\"device\":\"" + String(NOME) + 
                         "\",\"temperatura\":" + String(temperatura) + 
                         ",\"temperatura_min\":" + String(temperatura_min) + 
                         ",\"timestamp\":\"" + String(ora_locale) + ":" + String(minuti_locali) + "\"}";
        
        if (publishMQTT(MQTT_STATUS_TOPIC, response.c_str(), false)) {
            #if DEBUG
                spl("Temperatura inviata con successo");
            #endif
        } else {
            spl("Errore invio temperatura");
        }
        return;
    }

    // Parsing degli altri comandi
    if (doc["command"].is<const char*>()) {
        const char* command = doc["command"];
        spl("Comando ricevuto: " + String(command));
        
        if (strcmp(command, "restart") == 0) {
            spl("Riavvio dispositivo...");
            ESP.restart();
        }
        else if (strcmp(command, "status") == 0) {
            String statusMsg = "{\"device\":\"" + String(NOME) + 
                             "\",\"timestamp\":\"" + String(ora_locale) + ":" + String(minuti_locali) + 
                             "\",\"signal\":" + String(modem.getSignalQuality()) + "}";
            publishMQTT(MQTT_STATUS_TOPIC, statusMsg.c_str());
        }
        // ... altri comandi ...
    }
}

void setupMQTT() {
    spl("Inizializzazione MQTT...");
    mqttClient.setBufferSize(512);  // Aumenta il buffer
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(onMqttMessage);
    mqttClient.setKeepAlive(60);
    spl("Setup MQTT completato. Buffer size: " + String(mqttClient.getBufferSize()));
}

void handleMQTTConnection() {
    if (!mqttClient.connected()) {
        spl("Connessione MQTT...");
        String clientId = MQTT_CLIENT_ID;
        
        #if DEBUG
            spl("Server: " + String(mqtt_server));
            spl("Porta: " + String(mqtt_port));
            spl("Client ID: " + clientId);
        #endif
        
        if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
            spl("MQTT Connesso!");
            mqttClient.subscribe(MQTT_COMMAND_TOPIC);
            
            String connectMsg = "{\"device\":\"" + String(MQTT_CLIENT_ID) + "\",\"status\":\"online\"}";
            mqttClient.publish(MQTT_STATUS_TOPIC, connectMsg.c_str(), true);
        } else {
            spl("Fallita connessione MQTT, rc=" + String(mqttClient.state()));
            #if DEBUG
                // Debug stato errore
                switch(mqttClient.state()) {
                    case -4: spl("MQTT_CONNECTION_TIMEOUT"); break;
                    case -3: spl("MQTT_CONNECTION_LOST"); break;
                    case -2: spl("MQTT_CONNECT_FAILED"); break;
                    case -1: spl("MQTT_DISCONNECTED"); break;
                    case 0: spl("MQTT_CONNECTED"); break;
                    case 1: spl("MQTT_CONNECT_BAD_PROTOCOL"); break;
                    case 2: spl("MQTT_CONNECT_BAD_CLIENT_ID"); break;
                    case 3: spl("MQTT_CONNECT_UNAVAILABLE"); break;
                    case 4: spl("MQTT_CONNECT_BAD_CREDENTIALS"); break;
                    case 5: spl("MQTT_CONNECT_UNAUTHORIZED"); break;
                    default: spl("MQTT_UNKNOWN_ERROR"); break;
                }
            #endif
        }
    }
}

void handleMQTTMessages() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
}

bool publishMQTT(const char* topic, const char* payload, bool retain) {
    if (!mqttClient.connected()) {
        spl("MQTT non connesso, messaggio non pubblicato");
        return false;
    }
    
    bool result = mqttClient.publish(topic, payload, retain);
    
    #if DEBUG
        spl("Tentativo pubblicazione MQTT:");
        spl("Topic: " + String(topic));
        spl("Payload: " + String(payload));
        spl("Retain: " + String(retain));
        spl("Risultato: " + String(result ? "successo" : "fallito"));
        spl("Stato client MQTT: " + String(mqttClient.state()));
    #endif
    
    return result;
} 