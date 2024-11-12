#include "temperatura.h"
#include "utilities.h"
#include "config.h"
#include "funzioni.h"

/***************************
* Temperature
****************************/
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature s_temperatura(&oneWire);  

float temperatura;
float temperatura_min = 100;
unsigned long intervalloPubTemp = 600000;

void setupTemperature() {
    s_temperatura.begin();
    s_temperatura.setResolution(11);
    s_temperatura.setWaitForConversion(false);  // Modalità asincrona
    spl("Setup temperatura completato");
}

void readTemperature() {
    static int lastResetDay = -1;
    // Richiesta temperatura in modo asincrono
    s_temperatura.requestTemperatures();
    
    // Lettura temperatura
    float nuova_temperatura = s_temperatura.getTempCByIndex(0);
    if (nuova_temperatura != DEVICE_DISCONNECTED_C) {
        temperatura = nuova_temperatura;
        spl("Temperatura: " + String(temperatura));

        // Reset giornaliero temperatura minima alle 12:00
        if (ora_locale == 12 && minuti_locali == 0 && lastResetDay != giorno_anno) {
            spl("Reset giornaliero temperatura minima");
            temperatura_min = temperatura;
            lastResetDay = giorno_anno;
        }
        
        // Aggiorna temperatura minima se necessario
        if (temperatura < temperatura_min) {
            temperatura_min = temperatura;
            spl("Temperatura min: " + String(temperatura_min));    
        }
        
        errore_t = false;
        
        // Aggiorna intervallo di pubblicazione in base alla temperatura
        if (periodoBrinate) {
            if (temperatura < 2) {
                intervalloPubTemp = 60000;      // 1 minuto
            } else if (temperatura < 5) {
                intervalloPubTemp = 120000;     // 2 minuti
            } else {
                intervalloPubTemp = 300000;     // 5 minuti
            }
        } else {
            intervalloPubTemp = 600000;         // 10 minuti
        }
    } else {
        errore_t = true;
        spl("Errore lettura temperatura");
    }
    
    pubblicaStatoErrore(errore_t, false, false);
}

void pubblicaTemperatura(float temp) {
    if (publishMQTT(TOPIC_TEMPERATURA, String(temp).c_str())) {
        spl("Temperatura pubblicata: " + String(temp));
    } else {
        spl("Errore nella pubblicazione della temperatura");
    }
}

void pubblicaTemperaturaMinima(float tempMin) {
    if (tempMin != 100) {
        if (publishMQTT(TOPIC_TEMP_MIN, String(tempMin).c_str())) {
            spl("Temperatura minima pubblicata: " + String(tempMin));
        } else {
            spl("Errore nella pubblicazione della temperatura minima");
        }
    }
}
