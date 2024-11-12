#include "funzioni.h"
#include "config.h"
#include "utilities.h"
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <OneWire.h>


/***************************
* Temperature
****************************/
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature s_temperatura(&oneWire);      

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

/***************************
* INA226
****************************/
#define SDA     13
#define SCL     21

float voltaggio = 0;
bool accesa = false;
float setpoint = 0;
bool allarme = false;
float tempo_accensione = 0;
float tempo_totale = 0;

void statoVentola() {

    publishMQTT(TOPIC_STATO_VENTOLA, accesa ? "on" : "off");

}


/***************************
* CheckConnessioni
****************************/
void checkConnessioni() {
    int retryCount = 0;
    const int maxRetries = 5;

    while (!modem.isNetworkConnected() && retryCount < maxRetries) {
        spl("Network non connesso. Tentativo di riconnessione...");
        if (!modem.restart()) {
            spl("Errore nel riavvio del modem");
            retryCount++;
            //esp_task_wdt_reset();
            delay(CONNECTION_RETRY_DELAY);  // Attendi 5 secondi prima di riprovare
            continue;
        }
        if (!modem.waitForNetwork()) {
            spl("Errore nella riconnessione alla rete");
            retryCount++;
            //esp_task_wdt_reset();
            delay(CONNECTION_RETRY_DELAY);  // Attendi 5 secondi prima di riprovare
            continue;
        }
        spl("Rete riconnessa con successo");
    }

    if (retryCount >= maxRetries) {
        spl("Impossibile connettersi alla rete dopo 5 tentativi. Riavvio...");
        //ESP.restart();
    }

    retryCount = 0;  // Resetta il contatore per la connessione GPRS

    while (!modem.isGprsConnected() && retryCount < maxRetries) {
        spl("GPRS non connesso. Tentativo di riconnessione...");
        if (!modem.gprsConnect(APN, "", "")) {  // Usa il tuo APN, username e password
            spl("Errore nella connessione GPRS");
            retryCount++;
            //esp_task_wdt_reset();
            delay(CONNECTION_RETRY_DELAY);  // Attendi 5 secondi prima di riprovare
            continue;
        }
        spl("GPRS riconnesso con successo");
    }

    if (retryCount >= maxRetries) {
        spl("Impossibile connettersi al GPRS dopo 5 tentativi. Riavvio...");
        //ESP.restart();
    }
}

/***************************
* TIME Setup
****************************/
// Variabili globali per memorizzare le informazioni temporali
int giorno_anno = 0; 
int ora_locale = 0;
int minuti_locali = 0;
bool is_ora_legale = false;

int giorno_inizio_brinate = DEFAULT_GIORNO_INIZIO_BRINATE; // 1 marzo
int giorno_fine_brinate = DEFAULT_GIORNO_FINE_BRINATE;  // 1 maggio
bool periodoBrinate = false;
bool lastPeriodoBrinateState = false;

bool isOrarioLegale(int giorno, int mese, int ora) {
    // L'ora legale in Italia inizia l'ultima domenica di marzo e termina l'ultima domenica di ottobre
    if (mese < 3 || mese > 10) {
        return false;
    }
    if (mese > 3 && mese < 10) {
        return true;
    }
    int ultimaDomenica = 31 - ((5 + 31 - giorno) % 7);
    if (mese == 3) {
        return (giorno >= ultimaDomenica && ora >= 2) || giorno > ultimaDomenica;
    } else {  // mese == 10
        return giorno < ultimaDomenica || (giorno == ultimaDomenica && ora < 3);
    }
}

void aggiornaDataOra() {
    String res;
    
    // Richiedi l'ora dalla rete
    modem.sendAT("+CCLK?");
    if (modem.waitResponse(1000L, res) == 1) {
        // Formato risposta: +CCLK: "YY/MM/DD,HH:MM:SS±ZZ"
        if (res.indexOf("+CCLK: ") != -1) {
            // Estrai la stringa dell'ora
            String timeStr = res.substring(res.indexOf("\"") + 1, res.lastIndexOf("\""));
            
            // Parsing della stringa
            int year = timeStr.substring(0,2).toInt() + 2000;
            int month = timeStr.substring(3,5).toInt();
            int day = timeStr.substring(6,8).toInt();
            int hour = timeStr.substring(9,11).toInt();
            int min = timeStr.substring(12,14).toInt();
            int sec = timeStr.substring(15,17).toInt();
            
            // Aggiorna le variabili globali
            ora_locale = hour;
            minuti_locali = min;
            
            // Calcola il giorno dell'anno
            struct tm timeinfo = {0};
            timeinfo.tm_year = year - 1900;
            timeinfo.tm_mon = month - 1;
            timeinfo.tm_mday = day;
            mktime(&timeinfo);
            giorno_anno = timeinfo.tm_yday + 1;
            
            // Determina se è in vigore l'ora legale
            is_ora_legale = isOrarioLegale(day, month, hour);
            
            spl("Data e ora aggiornate: " + String(day) + "/" + String(month) + "/" + String(year) + 
                " " + String(hour) + ":" + String(min) + ":" + String(sec));
            spl("Giorno dell'anno: " + String(giorno_anno));
            spl("Ora legale in vigore: " + String(is_ora_legale ? "Sì" : "No"));
        }
    } else {
        spl("Errore nella lettura dell'ora dal modem");
    }
}

void aggiornaDataOraLocale() {
    minuti_locali++;
    if (minuti_locali >= 60) {
        minuti_locali = 0;
        ora_locale++;
        if (ora_locale >= 24) {
            ora_locale = 0;
            giorno_anno++;
            if (giorno_anno > 366) {
                giorno_anno = 1;
            }
        }

        // Aggiorna l'ora legale se necessario (solo a mezzanotte)
        if (ora_locale == 0 && minuti_locali == 0) {
            is_ora_legale = isOrarioLegale(1, (giorno_anno <= 31) ? 1 : ((giorno_anno <= 59) ? 2 : 3), 0);
        } 
    }
    
    // Stampa le informazioni aggiornate
    spl("Ora locale aggiornata: " + String(ora_locale) + ":" + String(minuti_locali));
    spl("Giorno dell'anno: " + String(giorno_anno));
    
    // Aggiorna lo stato del periodo brinate
    bool newPeriodoBrinate = (giorno_anno >= giorno_inizio_brinate && giorno_anno <= giorno_fine_brinate);
    
    // Controlla se stiamo entrando nel periodo brinate
   if (newPeriodoBrinate && !lastPeriodoBrinateState) {
        tempo_totale = 0;  // Reset del tempo totale
        spl("Inizio periodo brinate - Reset tempo totale");
        
        // Pubblica il reset del tempo totale
       // publishWithRetry(TOPIC_TEMPO_TOTALE, "0", true); ------------------------ da riattivare o cambiare
    } 

    // Aggiorna le variabili
    periodoBrinate = newPeriodoBrinate;
    lastPeriodoBrinateState = newPeriodoBrinate;
}

/***************************
 * Errori
****************************/
bool errore_t = false;
bool errore_b = false;
bool errore_i = false;

// Codici di errore
enum CodiceErrore {
    NESSUN_ERRORE = 0,
    ERRORE_TEMPERATURA = 1,
    BATTERIA_BASSA = 2,
    ERRORE_INVIO_DATI = 3
};

// Variabile globale per tenere traccia dell'ultimo errore pubblicato
static int ultimoErrorePubblicato = NESSUN_ERRORE;

void pubblicaStatoErrore(bool errore_temperatura, bool errore_batteria, bool errore_invio) {
    static int ultimo_codice_errore = -1;  // -1 indica primo avvio
    
    int codice_errore = 0;  // 0 = nessun errore
    
    if (errore_temperatura) codice_errore = 1;      // Errore Temperatura
    else if (errore_batteria) codice_errore = 2;    // Batteria Bassa
    else if (errore_invio) codice_errore = 3;       // Errore Invio Dati
    
    // Pubblica solo se lo stato è cambiato o è il primo avvio
    if (codice_errore != ultimo_codice_errore) {
        #if DEBUG
            spl("Cambio stato errore: " + String(ultimo_codice_errore) + " -> " + String(codice_errore));
            if (codice_errore == 0) {
                spl("Nessun errore presente");
            } else {
                spl("Nuovo codice errore: " + String(codice_errore));
            }
        #endif
        
        if (publishMQTT(TOPIC_ERRORI, String(codice_errore).c_str(), true)) {
            ultimo_codice_errore = codice_errore;  // Aggiorna solo se la pubblicazione è riuscita
            #if DEBUG
                spl("Nuovo stato errore pubblicato");
            #endif
        } else {
            spl("Errore pubblicazione stato errore");
        }
    }
    #if DEBUG
        else {
            spl("Stato errore non cambiato, nessuna pubblicazione");
        }
    #endif
}

void checkErrors() {
    // Controlla e pubblica lo stato degli errori
    pubblicaStatoErrore(errore_t, errore_b, errore_i);
}

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
    snprintf(TOPIC_CMD, 50, "%s%s/cmd", MQTT_GROUP, NOME);
    snprintf(TOPIC_TEST, 50, "%s%s/test", MQTT_GROUP, NOME);
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
        doc["val_tpl"] = "{% set map = {'0': 'Tutto OK!','1': 'Errore Temperatura!','2': 'Batteria Bassa!','3': 'Errore Invio Dati!'} %} {{ map[value] if value in map else 'error' }}"; 
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
        statoVentola();
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
        
        spl("Dispositivo Rimosso!");
    }
}

/***************************
 * Temperature
****************************/

float temperatura;
float temperatura_min = 100;
unsigned long intervalloPubTemp = 600000;

void pubblicaTemperaturaMinima(float tempMin) {
    if (tempMin != 100) {
        if (publishMQTT(TOPIC_TEMP_MIN, String(tempMin).c_str())) {
            spl("Temperatura minima pubblicata: " + String(tempMin));
    } else {
        spl("Errore nella pubblicazione della temperatura minima");
        }
    }
}

void pubblicaTemperatura(float temp) {
    if (publishMQTT(TOPIC_TEMPERATURA, String(temp).c_str())) {
        spl("Temperatura pubblicata: " + String(temp));
    } else {
        spl("Errore nella pubblicazione della temperatura");
    }
    
}

/***************************
 * Timer
****************************/
// Variabili per il timer
bool isCountingTime = false;
unsigned long startCountingTime = 0;
unsigned long lastPublishTime = 0;
int lastDay = 0;           // Per tracciare il cambio giorno
int lastResetHour = 16;    // Ora di reset

void checkAndUpdateTimer() {
    // Verifica se siamo nel periodo di conteggio (16:00 - 10:00 del giorno dopo)
    bool isTimeWindow = (ora_locale >= 16 || ora_locale < 10);
    
    // Reset giornaliero alle 16:00
    if (ora_locale == 16 && ora_locale != lastResetHour) {
        tempo_accensione = 0;  // Reset solo alle 16:00
        lastResetHour = ora_locale;
        spl("Reset giornaliero del tempo di accensione");
    }
    
    // Se tutte le condizioni sono soddisfatte
    if (periodoBrinate && isTimeWindow) {
        if (accesa) {
            if (!isCountingTime) {
                // Inizia un nuovo periodo di conteggio
                isCountingTime = true;
                startCountingTime = millis();
                spl("Inizio nuovo periodo di conteggio");
            }
            
            // Calcola il tempo trascorso in questo periodo di accensione
            unsigned long currentPeriod = (millis() - startCountingTime) / 1000.0;
            
            // Aggiorna sia tempo_accensione che tempo_totale
            tempo_accensione += currentPeriod;
            tempo_totale += currentPeriod;
            
            // Reset del contatore per il prossimo calcolo
            startCountingTime = millis();
            
            // Pubblica ogni 5 minuti
            if (millis() - lastPublishTime >= 300000) {  // 5 minuti = 300000 ms
                lastPublishTime = millis();
                
                // Pubblica tempo_accensione (periodo 16-10)
                if (publishMQTT(TOPIC_TEMPO_ACCESA, String(tempo_accensione).c_str())) {
                    spl("Tempo accensione periodo corrente: " + String(tempo_accensione) + " secondi");
                }
                
                // Pubblica tempo_totale (tutto il periodo brinate)
                if (publishMQTT(TOPIC_TEMPO_TOTALE, String(tempo_totale).c_str())) {
                    spl("Tempo totale periodo brinate: " + String(tempo_totale) + " secondi");
                }
            }
        } else if (isCountingTime) {
            // La ventola si è spenta, termina questo periodo di conteggio
            isCountingTime = false;
            spl("Fine periodo di conteggio");
        }
    } else if (isCountingTime) {
        // Fuori dalla finestra temporale, termina il conteggio
        isCountingTime = false;
        spl("Fuori dalla finestra temporale 16-10");
    }
} 
