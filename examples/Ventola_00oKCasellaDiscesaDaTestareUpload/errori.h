#pragma once
#include <Arduino.h>
#include "config.h"
#include "mqtt_handler.h"



// Variabili globali
extern bool errore_t;
extern bool errore_b;
extern bool errore_a;

/***************************
 * Errori
****************************/
//void checkErrors();
void pubblicaStatoErrore();
