#include <Arduino.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
#include "utilities.h"
#include "config.h"
#include "data_ora.h"
#include "mqtt_handler.h"
#include "tasks.h"
#include "modem.h"

XPowersPMU PMU;

void setup() {
    if (DEBUG) {
        Serial.begin(115200);
        while (!Serial);
        delay(3000);
    }
    
    // Setup Power Management
    spl("Setup Power");
    spl("=========================================");
    if (!PMU.begin(Wire1, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
        Serial.println("ERROR: Init PMU failed!");
        return;
    }
    //Modem 2700~3400mV VDD
    PMU.setDC3Voltage(3000);
    PMU.enableDC3();
    // TS Pin detection must be disable, otherwise it cannot be charged
    PMU.disableTSPinMeasure();
    spl("Power setup Terminato");
    spl("=========================================");
    
    // Setup e inizializzazione modem
    setupModem();
    initModem();
    setupNetwork();
    
    // Aggiorna l'ora prima di tutto il resto
    spl("Aggiornamento ora iniziale...");
    if (aggiornaDataOraOnline()) {
        spl("Ora inizializzata correttamente");
    } else {
        spl("ERRORE: Impossibile ottenere l'ora");
    }
    
    // Inizializzazione MQTT e altri servizi
    initMQTTTopics();
    setupMQTT();
    
    // Inizializza temperatura
    setupTemperature();
    
    // Inizializza voltaggi
    setupVoltaggi();
    
    // Inizializza i task FreeRTOS
    startFreeRTOSTasks();
    
    // Stampa informazioni sulla memoria
    spl("\n--- Stato Memoria ---");
    spl("RAM totale: " + String(ESP.getHeapSize()) + " bytes");
    spl("RAM libera: " + String(ESP.getFreeHeap()) + " bytes");
    spl("RAM minima libera: " + String(ESP.getMinFreeHeap()) + " bytes");
    spl("RAM massima allocata: " + String(ESP.getMaxAllocHeap()) + " bytes");
    spl("PSRAM totale: " + String(ESP.getPsramSize()) + " bytes");
    spl("PSRAM libera: " + String(ESP.getFreePsram()) + " bytes");
    spl("PSRAM minima libera: " + String(ESP.getMinFreePsram()) + " bytes");
    spl("PSRAM massima allocata: " + String(ESP.getMaxAllocPsram()) + " bytes");
    spl("--------------------\n");
    
    spl("Setup Terminato");
    spl("=========================================");
}

void loop() {
    vTaskDelete(NULL);
}
