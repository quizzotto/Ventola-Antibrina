#pragma once
#include <Arduino.h>
#include "config.h"
#include "mqtt_handler.h"
#include "temperatura.h"
#include "modem.h"
#include "utilities.h"
#include "voltaggi.h"
#include "discovery.h"

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
bool aggiornaDataOraOnline();
/***************************
* Timer
****************************/
void checkAndUpdateTimer();



