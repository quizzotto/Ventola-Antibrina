#include "mqtt_handler.h"
#include <HTTPClient.h>
#include <Update.h>

// Configurazione MQTT
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;

static void avviaOTA(String url);
// Dichiarazione del client MQTT
PubSubClient mqttClient(client);

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    // Converti il payload in una stringa
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    String comando = String(message);
    spl("Messaggio MQTT ricevuto [" + String(topic) + "]: " + comando);

    // Gestione comandi
    if (comando == "Temperatura") {
        String response = String("Temperatura ")
                          + String(NOME)
                          + String(": ")
                          + String(temperatura);
        if (publishMQTT(TOPIC_STATUS, response.c_str())) {
            spl("Temperatura inviata: " + response);
        } else {
            spl("Errore invio temperatura");
        }
    }
    else if (comando == "DiscoveryOn") {
        auto_discovery = true;
        spl("Discovery attivato");
        haDiscovery();
    }
    else if (comando == "DiscoveryOff") {
        auto_discovery = false;
        spl("Discovery disattivato");
        haDiscovery();
    }
    else if (comando == "Riavvia") {
        spl("Riavvio dispositivo...");
        ESP.restart();
    }
    else if (comando == "InviaStato") {
        spl("Invio stato...");
        pubblicaStatoErrore();
        pubblicaTemperatura(temperatura);
        pubblicaStatoVentola(accesa);
        pubblicaVoltaggi(voltaggio_batteria);
    }
    else if (comando.startsWith("ota:")) {
        String url = comando.substring(4); // Estrae l'URL dopo "ota:"
        spl("Avvio aggiornamento OTA da: " + url);
        avviaOTA(url);
    }
    else {
        spl("Comando non riconosciuto: " + comando);
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
            mqttClient.subscribe(TOPIC_CMD);
            
            String connectMsg = "{\"device\":\"" + String(MQTT_CLIENT_ID) + "\",\"status\":\"online\"}";
            mqttClient.publish(TOPIC_STATUS, connectMsg.c_str(), true);
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

void avviaOTA(String url) {
    if (!modem.isNetworkConnected()) {
        publishMQTT(TOPIC_STATUS, "Errore OTA: rete non connessa", false);
        return;
    }

    publishMQTT(TOPIC_STATUS, "Inizio download firmware...", false);
    
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int totalLength = http.getSize();
        if (totalLength <= 0) {
            publishMQTT(TOPIC_STATUS, "Errore OTA: dimensione firmware non valida", false);
            return;
        }

        if (Update.begin(totalLength)) {
            size_t written = Update.writeStream(http.getStream());
            
            if (written == totalLength) {
                if (Update.end()) {
                    publishMQTT(TOPIC_STATUS, "Aggiornamento completato. Riavvio...", false);
                    delay(1000);
                    ESP.restart();
                } else {
                    publishMQTT(TOPIC_STATUS, "Errore durante la finalizzazione dell'aggiornamento", false);
                }
            } else {
                publishMQTT(TOPIC_STATUS, "Errore durante la scrittura del firmware", false);
            }
        } else {
            publishMQTT(TOPIC_STATUS, "Non abbastanza spazio per l'aggiornamento", false);
        }
    } else {
        String errorMsg = "Errore download firmware: " + String(httpCode);
        publishMQTT(TOPIC_STATUS, errorMsg.c_str(), false);
    }
    
    http.end();
} 