#include "voltaggi.h"


INA226_WE ina226_batt(INA226_ADDR_BATT);
INA226_WE ina226_vent(INA226_ADDR_VENT);

float voltaggio_batteria = 0;
float voltaggio_ventola = 0;
bool accesa = false;
float tempo_accensione = 0;
float tempo_totale = 0;

void setupVoltaggi() {
    Wire.begin(SDA, SCL);
    
    // Setup INA226 per batteria
    if (!ina226_batt.init()) {
        spl("Errore inizializzazione INA226 batteria!");
        return;
    }
    ina226_batt.waitUntilConversionCompleted();

    // Setup INA226 per ventola
    if (!ina226_vent.init()) {
        spl("Errore inizializzazione INA226 ventola!");
        return;
    }
    ina226_vent.waitUntilConversionCompleted();

    spl("Setup voltaggi completato");
}

void readVoltaggi() {
    // Lettura voltaggio batteria
    voltaggio_batteria = ina226_batt.getBusVoltage_V();
    voltaggio_ventola = ina226_vent.getBusVoltage_V();  // Leggi sempre il voltaggio ventola
    
    // Controllo voltaggio batteria solo se è collegata (voltaggio significativo)
    if (voltaggio_batteria > 0.1) {  // Batteria collegata
        if (voltaggio_batteria <= batteria && !errore_b) {
            errore_b = true;
            errore_a = false;  // Reset altri errori
            spl("Errore: batteria bassa (" + String(voltaggio_batteria) + "V)");
        } else {
            errore_b = false;
            
            // Aggiorna stato ventola
            bool nuovo_stato = (voltaggio_ventola > VOLTAGE_THRESHOLD);
            if (nuovo_stato != accesa) {
                accesa = nuovo_stato;
                pubblicaStatoVentola(accesa);
            }
            
            // Controllo errore alternatore solo se non ci sono altri errori
            if (voltaggio_batteria > VOLTAGE_THRESHOLD && 
                voltaggio_ventola < ALTERNATORE_VOLTAGE_THRESHOLD && 
                accesa) {
                errore_a = true;
                spl("Errore: alternatore (" + String(voltaggio_ventola) + "V)");
            } else {
                errore_a = false;
            }
        }
    } else {  // Batteria non collegata
        errore_b = false;
        errore_a = false;
        
        // Aggiorna stato ventola anche quando la batteria non è collegata
        bool nuovo_stato = (voltaggio_ventola > VOLTAGE_THRESHOLD);
        if (nuovo_stato != accesa) {
            accesa = nuovo_stato;
            pubblicaStatoVentola(accesa);
        }
    }
    
    #if DEBUG
        spl("Voltaggio batteria: " + String(voltaggio_batteria) + "V");
        spl("Voltaggio ventola: " + String(voltaggio_ventola) + "V");
        spl("Stato ventola: " + String(accesa ? "ON" : "OFF"));
    #endif
}

void pubblicaVoltaggi(float voltaggio) {
    if (publishMQTT(TOPIC_BATTERIA, String(voltaggio).c_str())) {
        spl("Voltaggio batteria pubblicato: " + String(voltaggio));
    }
}

void pubblicaStatoVentola(bool stato) {
    publishMQTT(TOPIC_STATO_VENTOLA, stato ? "on" : "off");
    spl("Pubblicato nuovo stato ventola: " + String(stato ? "on" : "off"));
}