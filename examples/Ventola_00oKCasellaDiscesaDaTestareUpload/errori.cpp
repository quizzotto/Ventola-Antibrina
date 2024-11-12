#include "errori.h"

/***************************
 * Errori
****************************/

// Codici di errore
enum CodiceErrore {
    NESSUN_ERRORE = 0,
    ERRORE_TEMPERATURA = 1,
    BATTERIA_BASSA = 2,
    ERRORE_INVIO_DATI = 3
};

bool errore_t = false;
bool errore_b = false;
bool errore_a = false;

// Variabile globale per tenere traccia dell'ultimo errore pubblicato
static int ultimoErrorePubblicato = NESSUN_ERRORE;

void pubblicaStatoErrore() {
    static bool last_errore_b = false;
    static bool last_errore_t = false;
    static bool last_errore_a = false;
    static int ultimo_codice_errore = -1;
    
    // Verifica se c'è stato un cambiamento negli errori
    if (errore_b != last_errore_b || 
        errore_t != last_errore_t || 
        errore_a != last_errore_a) {
        
        // Determina il codice errore (priorità: temperatura > batteria > alternatore)
        int codice_errore = 0;
        if (errore_t) codice_errore = 1;
        else if (errore_b) codice_errore = 2;
        else if (errore_a) codice_errore = 3;
        
        // Pubblica solo se il codice è cambiato
        if (codice_errore != ultimo_codice_errore) {
            if (publishMQTT(TOPIC_ERRORI, String(codice_errore).c_str(), true)) {
                #if DEBUG
                    if (codice_errore == 0) {
                        spl("Errori risolti");
                    } else {
                        spl("Nuovo errore: " + String(codice_errore));
                        switch(codice_errore) {
                            case 1: spl("- Errore temperatura"); break;
                            case 2: spl("- Errore batteria"); break;
                            case 3: spl("- Errore alternatore"); break;
                        }
                    }
                #endif
                ultimo_codice_errore = codice_errore;
            }
        }
        
        // Aggiorna gli stati precedenti
        last_errore_b = errore_b;
        last_errore_t = errore_t;
        last_errore_a = errore_a;
    }
}


