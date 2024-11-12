#include <Arduino.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
#include "utilities.h"
#include "config.h"
#include "funzioni.h"
#include "mqtt_handler.h"
#include <TaskScheduler.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "temperatura.h"
XPowersPMU PMU;

/***************************
* Modem Setup
****************************/
#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_MODEM_SIM7080
#include <TinyGsmClient.h>
#define SerialAT Serial1
TinyGsm modem(SerialAT); 
TinyGsmClient client(modem);

const char *register_info[] = {
    "Not registered, MT is not currently searching an operator to register to.The GPRS service is disabled, the UE is allowed to attach for GPRS if requested by the user.",
    "Registered, home network.",
    "Not registered, but MT is currently trying to attach or searching an operator to register to. The GPRS service is enabled, but an allowable PLMN is currently not available. The UE will start a GPRS attach as soon as an allowable PLMN is available.",
    "Registration denied, The GPRS service is disabled, the UE is not allowed to attach for GPRS if it is requested by the user.",
    "Unknown.",
    "Registered, roaming.",
};
enum {
    MODEM_CATM = 1,
    MODEM_NB_IOT,
    MODEM_CATM_NBIOT,
};

/***************************
* Task Scheduler
****************************/
Scheduler ts;

// Callback functions
void aggiornaDataCallback() {
    aggiornaDataOraLocale();
}

void publishTelemetryCallback() {
    String message = "{\"device\":\"" + String(NOME) + 
                     "\",\"timestamp\":\"" + String(ora_locale) + ":" + String(minuti_locali) + 
                     "\",\"signal\":" + String(modem.getSignalQuality()) + "}";
    spl (message);
    spl(temperatura);
}

// Callback functions per i task
void mqttConnectionCallback() {
    checkConnessioni();  // Verifica connessione modem/rete
    handleMQTTConnection();  // Gestione connessione MQTT
}
void mqttMessageCallback() {
    handleMQTTMessages();  // Gestione messaggi MQTT
}
void checkTemperaturaCallback() {
    startTempConversion();  // Avvia la conversione della temperatura
}
void readTemperaturaCallback() {  
    readTemperature();  // Legge la temperatura
}
void publishTemperaturaCallback() {
    pubblicaTemperatura(temperatura);  // Pubblica la temperatura
    pubblicaTemperaturaMinima(temperatura_min);  // Pubblica la temperatura minima
}    
void checkAndUpdateTimerCallback() {
    checkAndUpdateTimer();  // Controlla e aggiorna il timer
}
void checkErrorsCallback() {
    checkErrors();  // Controlla gli errori
}
void statoVentolaCallback() {
    statoVentola();  // Controlla lo stato della ventola
}

// Definizione tasks
Task tAggiornaData(60000, TASK_FOREVER, &aggiornaDataCallback);         // Aggiorna data e ora locale ogni 60s
Task tMqttConnection(CONNECTION_RETRY_DELAY, TASK_FOREVER, &mqttConnectionCallback);    // Controlla connessione ogni 10s
Task tMqttMessages(100, TASK_FOREVER, &mqttMessageCallback);         // Gestisce messaggi ogni 100ms
Task tPublishTelemetry(30000, TASK_FOREVER, &publishTelemetryCallback); // ogni 30s
Task tCheckTemperatura(TEMP_CHECK_INTERVAL, TASK_FOREVER, &checkTemperaturaCallback);  // Controlla la temperatura ogni 10s
Task tReadTemperatura(TEMP_CONVERSION_TIME, TASK_ONCE, &readTemperaturaCallback);  // Legge la temperatura
Task tPublishTemperatura(intervalloPubTemp, TASK_FOREVER, &publishTemperaturaCallback);  // Pubblica la temperatura
Task tCheckAndUpdateTimer(1000, TASK_FOREVER, &checkAndUpdateTimerCallback);  // Controlla e aggiorna il timer ogni 1s
Task tCheckErrors(10000, TASK_FOREVER, &checkErrorsCallback);  // Controlla gli errori ogni 10s
Task tStatoVentola(600000, TASK_FOREVER, &statoVentolaCallback);  // Controlla lo stato della ventola ogni 600s
/***************************
* Fine Task Scheduler
****************************/


void setup() {
    if (DEBUG) {
        Serial.begin(115200);
        while (!Serial);
        delay(3000);
    }
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
    
    // Setup del modem
    spl("Setup Modem");
    Serial1.begin(115200, SERIAL_8N1, BOARD_MODEM_RXD_PIN, BOARD_MODEM_TXD_PIN);
    pinMode(BOARD_MODEM_PWR_PIN, OUTPUT);
    pinMode(BOARD_MODEM_DTR_PIN, OUTPUT);
    pinMode(BOARD_MODEM_RI_PIN, INPUT);

    int retry = 0;
    while (!modem.testAT(1000)) {
        spl(".");
        if (retry++ > 5) {
            spl("Warn : try reinit modem!");
            // Pull down PWRKEY for more than 1 second according to manual requirements
            digitalWrite(BOARD_MODEM_PWR_PIN, LOW);
            delay(100);
            digitalWrite(BOARD_MODEM_PWR_PIN, HIGH);
            delay(1000);
            digitalWrite(BOARD_MODEM_PWR_PIN, LOW);
            retry = 0;
        }
    }
    spl("Modem started!");
    
    // Verifica SIM con tentativi
    uint8_t sim_tentativi = 0;
    const uint8_t MAX_TENTATIVI = 3;
    const unsigned long ATTESA_RIAVVIO = 5000; // 5 secondi prima del riavvio
    
    while (modem.getSimStatus() != SIM_READY) {
        spl("SIM Card non rilevata!");
        sim_tentativi++;
        
        if (sim_tentativi >= MAX_TENTATIVI) {
            spl("SIM non rilevata dopo 3 tentativi.");
            spl("Riavvio dispositivo tra 5 secondi...");
            delay(ATTESA_RIAVVIO);
            ESP.restart();
        }
        
        delay(1000); // Attende 1 secondo prima del prossimo tentativo
    }
    
    // Disable RF
    modem.sendAT("+CFUN=0");
    if (modem.waitResponse(20000UL) != 1){
        spl("Disable RF Failed!");
    }
    /*********************************
     * step 4 : Set the network mode to NB-IOT
    ***********************************/
    modem.setNetworkMode(2);    //use automatic
    modem.setPreferredMode(MODEM_NB_IOT);
    uint8_t pre = modem.getPreferredMode();
    uint8_t mode = modem.getNetworkMode();
    spl("getNetworkMode:" + String(mode));
    spl("getPreferredMode:" + String(pre));
    //Set the APN manually. Some operators need to set APN first when registering the network.
    modem.sendAT("+CGDCONT=1,\"IP\",\"", APN, "\"");
    if (modem.waitResponse() != 1) {
        spl("Set operators apn Failed!");
        return;
    }
    //!! Set the APN manually. Some operators need to set APN first when registering the network.
    modem.sendAT("+CNCFG=0,1,\"", APN, "\"");
    if (modem.waitResponse() != 1) {
        spl("Config apn Failed!");
        return;
    }
    // Enable RF
    modem.sendAT("+CFUN=1");
    if (modem.waitResponse(20000UL) != 1) {
        spl("Enable RF Failed!");
    }
    /*********************************
    * step 5 : Wait for the network registration to succeed
    ***********************************/
    SIM70xxRegStatus s;
    do {
        s = modem.getRegistrationStatus();
        if (s != REG_OK_HOME && s != REG_OK_ROAMING) {
            sp(".");
            delay(1000);
        }

    } while (s != REG_OK_HOME && s != REG_OK_ROAMING) ;
    spl("");
    sp("Network register info:");
    spl(register_info[s]);
    // Activate network bearer, APN can not be configured by default,
    // if the SIM card is locked, please configure the correct APN and user password, use the gprsConnect() method
    if (!modem.isGprsConnected()) {
        modem.sendAT("+CNACT=0,1");
        if (modem.waitResponse() != 1) {
            spl("Activate network bearer Failed!");
            return;
        }
    }
    bool res = modem.isGprsConnected();
    sp("GPRS status: ");
    spl(res ? "connected" : "not connected");
    String imei = modem.getIMEI();
    sp("IMEI: ");
    spl(imei);
    String cop = modem.getOperator();
    sp("Operator: ");
    spl(cop);
    IPAddress local = modem.localIP();
    sp("Local IP: ");
    spl(local);
    int csq = modem.getSignalQuality();
    sp("Signal quality: ");
    spl(csq);
    modem.sendAT(GF("+CLTS=1"));
    if (modem.waitResponse(10000L) != 1) {
        spl("Enable Local Time Stamp Failed!");
        return;
    }
    // Before connecting, you need to confirm that the time has been synchronized.
    modem.sendAT("+CCLK?");
    modem.waitResponse(30000); 
    // Abilita l'aggiornamento automatico dell'ora dalla rete
    modem.sendAT("+CTZU=1");
    modem.waitResponse();
    modem.sendAT("+CLTS=1");
    modem.waitResponse();
    spl("Modem Setup Terminato");
    spl("=========================================");
    initMQTTTopics();
    aggiornaDataOra();
    
    setupMQTT();
    delay(1000);

      // Inizializzo  Temperatura
    s_temperatura.begin();
    s_temperatura.setResolution(11);

    // Setup dei task
    ts.addTask(tAggiornaData);
    ts.addTask(tMqttConnection);
    ts.addTask(tMqttMessages);
    ts.addTask(tPublishTelemetry);
    ts.addTask(tCheckTemperatura);  
    ts.addTask(tReadTemperatura);
    ts.addTask(tPublishTemperatura);
    ts.addTask(tCheckAndUpdateTimer);
    ts.addTask(tCheckErrors);
    ts.addTask(tStatoVentola);
    tAggiornaData.enable();
    tMqttConnection.enable();
    tMqttMessages.enable();
    tPublishTelemetry.enable();
    tCheckTemperatura.enable();
    tReadTemperatura.enable();
    tPublishTemperatura.enable();
    tCheckAndUpdateTimer.enable();
    tCheckErrors.enable();
    tStatoVentola.enable();
    spl("Tasks schedulati e abilitati");

    // Inizializza la ventola
    statoVentola();

    spl("Setup Terminato");
    spl("=========================================");
}

void loop() {
    ts.execute();
    delay(100);  // piccolo delay per evitare watchdog
}
