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


void startTempConversion() {
    s_temperatura.setWaitForConversion(false);
    s_temperatura.requestTemperatures();
    spl("Richiesta conversione temperatura");
    
}

void readTemperature() {
    spl("Lettura temperatura");
    static int lastResetDay = -1;  // Inizializza a -1 per forzare il primo reset
    // Controlla se è ora di resettare (12:00)
    if (ora_locale == 12 && minuti_locali == 0) {
        // Verifica se non abbiamo già fatto il reset oggi
        if (lastResetDay != giorno_anno) {
           
            spl("Reset giornaliero temperatura minima");
            spl("Vecchia temperatura minima: " + String(temperatura_min));
            
            temperatura_min = temperatura;  // Imposta la temperatura attuale come nuova minima
            lastResetDay = giorno_anno;         // Memorizza il giorno del reset
            
            spl("Nuova temperatura minima: " + String(temperatura_min));
        }
    }

    s_temperatura.setWaitForConversion(false);  // makes it async
    s_temperatura.requestTemperatures();
    s_temperatura.setWaitForConversion(true);




    float nuova_temperatura = s_temperatura.getTempCByIndex(0);
    if (nuova_temperatura != DEVICE_DISCONNECTED_C) {
        temperatura = nuova_temperatura;
        
        spl("Temperatura: " + String(temperatura));
        // Aggiorna la temperatura minima se necessario
        if (temperatura < temperatura_min) {
            temperatura_min = temperatura;
            spl("Temperatura min: " + String(temperatura_min));    
        }
        errore_t = false;  
    } else {
        errore_t = true;
        spl("Errore lettura temperatura");
    }

    if (periodoBrinate) {
        if (temperatura < 2) {
            intervalloPubTemp = 60000;  // 1 minuto
        } else if (temperatura < 5) {
            intervalloPubTemp = 120000;  // 2 minuti
        } else {
            intervalloPubTemp = 300000;  // 5 minuti
        }
    } else {
        intervalloPubTemp = 600000;  // 10 minuti
    }
    
    pubblicaStatoErrore(errore_t, false, false);
}
