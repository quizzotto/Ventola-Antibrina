#include <Arduino.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
#include "utilities.h"
#include "config.h"
#include "funzioni.h"
#include "mqtt_handler.h"
#include "tasks.h"
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

    // Inizializza temperatura
    setupTemperature();
    
    // Inizializza i task FreeRTOS
    startFreeRTOSTasks();

    spl("Setup Terminato");
    spl("=========================================");
}

void loop() {
    vTaskDelete(NULL); // Non necessario con FreeRTOS
}
