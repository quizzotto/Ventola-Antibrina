#pragma once
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGsmClient.h>
#include "config.h"
#include "mqtt_handler.h"
#include "temperatura.h"

/***************************
* MODEM 
****************************/
extern TinyGsm modem;
void checkConnessioni();

/***************************
 * TIME
****************************/
extern int ora_locale;
extern int minuti_locali;
extern int giorno_anno;
extern bool periodoBrinate;
bool isOrarioLegale(int giorno, int mese, int ora);
void aggiornaDataOra(); 
void aggiornaDataOraLocale();

/***************************
* Discovery
****************************/
extern bool auto_discovery;
void initMQTTTopics();
void haDiscovery();

/***************************
 * Timer
****************************/
void checkAndUpdateTimer();

/***************************
 * Errori
****************************/
extern bool errore_t;      // errore temperatura
extern bool errore_b;      // errore batteria
extern bool errore_i;      // errore invio
void checkErrors();
void pubblicaStatoErrore(bool errore_t, bool errore_b, bool errore_i);

/***************************
* Ventola
****************************/
void statoVentola();

