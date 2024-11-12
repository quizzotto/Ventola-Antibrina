#include "modem.h"
#include "utilities.h"
#include "config.h"

// Definizione variabili
TinyGsm modem(Serial1);
TinyGsmClient client(modem);

const char *register_info[] = {
    "Not registered, MT is not currently searching an operator to register to.The GPRS service is disabled, the UE is allowed to attach for GPRS if requested by the user.",
    "Registered, home network.",
    "Not registered, but MT is currently trying to attach or searching an operator to register to. The GPRS service is enabled, but an allowable PLMN is currently not available. The UE will start a GPRS attach as soon as an allowable PLMN is available.",
    "Registration denied, The GPRS service is disabled, the UE is not allowed to attach for GPRS if it is requested by the user.",
    "Unknown.",
    "Registered, roaming.",
};

void setupModem() {
    spl("Setup Modem");
    spl("=========================================");
    
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
    spl("Modem Avviato!");
    spl("=========================================");
}

void initModem() {
    spl("Inizializzazione Modem");
    spl("=========================================");
    // Verifica SIM con tentativi
    int sim_tentativi = 0;
    
    while (modem.getSimStatus() != SIM_READY) {
        spl("SIM Card non rilevata! Tentativo " + String(sim_tentativi + 1) + "/" + String(3));
        sim_tentativi++;
        
        if (sim_tentativi >= 3) {
            spl("SIM non rilevata dopo " + String(3) + " tentativi.");
            spl("Eseguo reset hardware in  10 secondi...");
            delay(10000);
            // Inizializza e attiva il watchdog
            ESP_ERROR_CHECK(esp_task_wdt_init(1, true));
            ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
            // Loop infinito che farà scattare il watchdog
            while(1) { 
                delay(100);
            }
        }
        delay(1000);
    }
    spl("SIM Card rilevata correttamente!");

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
    spl("Inizializzazione Modem Completata");
    spl("=========================================");
}

void setupNetwork() {
    spl("Setup Network");
    spl("=========================================");
    
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
    
    spl("Setup Network Completato");
    spl("=========================================");
} 

void checkConnessioni() {
    if (!modem.isNetworkConnected()) {
        spl("Network non connesso. Tentativo di riconnessione...");
        setupNetwork();
    }
    
    if (!modem.isGprsConnected()) {
        spl("GPRS non connesso. Tentativo di riconnessione...");
        modem.sendAT("+CNACT=0,1");
        if (modem.waitResponse() != 1) {
            spl("Attivazione bearer network fallita!");
            return;
        }
    }
}

