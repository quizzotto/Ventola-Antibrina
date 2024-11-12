#include "data_ora.h"


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

bool aggiornaDataOraOnline() {
    spl("Inizio aggiornamento ora online...");
    
    // Verifica connessione GPRS
    if (!modem.isGprsConnected()) {
        spl("GPRS non connesso, impossibile aggiornare ora");
        return false;
    }
    
    // Prova prima con AT
    spl("Tentativo aggiornamento via AT...");
    String res;
    modem.sendAT("+CCLK?");
    if (modem.waitResponse(2000L, res) == 1) {
        spl("Risposta AT ricevuta: " + res);
        if (res.indexOf("+CCLK: ") != -1) {
            String timeStr = res.substring(res.indexOf("\"") + 1, res.lastIndexOf("\""));
            spl("Stringa ora: " + timeStr);
            
            int year = timeStr.substring(0,2).toInt() + 2000;
            int month = timeStr.substring(3,5).toInt();
            int day = timeStr.substring(6,8).toInt();
            int hour = timeStr.substring(9,11).toInt();
            int min = timeStr.substring(12,14).toInt();
            
            // Verifica valori
            if (month < 1 || month > 12 || day < 1 || day > 31 || 
                hour > 23 || min > 59) {
                spl("Errore: Valori data/ora non validi");
                return false;
            }
            
            // Aggiorna variabili globali
            ora_locale = hour;
            minuti_locali = min;
            
            // Calcola giorno dell'anno
            struct tm timeinfo = {0};
            timeinfo.tm_year = year - 1900;
            timeinfo.tm_mon = month - 1;
            timeinfo.tm_mday = day;
            mktime(&timeinfo);
            giorno_anno = timeinfo.tm_yday + 1;
            
            // Calcola ora legale
            is_ora_legale = isOrarioLegale(day, month, hour);
            if (is_ora_legale) {
                ora_locale = (ora_locale + 1) % 24;
            }
            
            spl("Ora aggiornata: " + String(ora_locale) + ":" + String(minuti_locali));
            spl("Giorno anno: " + String(giorno_anno));
            spl("Ora legale: " + String(is_ora_legale ? "Sì" : "No"));
            return true;
        }
    }
    
    spl("Aggiornamento ora fallito");
    return false;
}

void aggiornaDataOra() {
    static unsigned long lastOnlineUpdate = 0;
    static bool firstUpdate = true;
    
    // Forza aggiornamento al primo avvio o ogni 24 ore
    if (firstUpdate || (millis() - lastOnlineUpdate >= 86400000)) {
        spl("Richiesto aggiornamento ora online");
        if (aggiornaDataOraOnline()) {
            lastOnlineUpdate = millis();
            firstUpdate = false;
            spl("Aggiornamento online completato");
        } else {
            spl("Errore aggiornamento ora online");
        }
    }
    
    // Aggiornamento locale
    aggiornaDataOraLocale();
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
    }
    
    // Aggiorna periodo brinate
    bool newPeriodoBrinate = (giorno_anno >= giorno_inizio_brinate && 
                             giorno_anno <= giorno_fine_brinate);
    
    // Gestione cambio periodo brinate
    if (newPeriodoBrinate != lastPeriodoBrinateState) {
        if (newPeriodoBrinate) {
            tempo_totale = 0;
            spl("Inizio periodo brinate - Reset tempo totale");
            publishMQTT(TOPIC_TEMPO_TOTALE, "0", true);
        } else {
            spl("Fine periodo brinate");
        }
        lastPeriodoBrinateState = newPeriodoBrinate;
    }
    
    periodoBrinate = newPeriodoBrinate;
    
    #if DEBUG
        static int last_printed_min = -1;
        if (minuti_locali != last_printed_min) {
            spl("Ora locale: " + String(ora_locale) + ":" + String(minuti_locali));
            spl("Giorno anno: " + String(giorno_anno));
            spl("Periodo brinate: " + String(periodoBrinate ? "Sì" : "No"));
            last_printed_min = minuti_locali;
        }
    #endif
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



