#pragma once

#include <Arduino.h>
#include "config.h"
#include "errori.h"
#include <Wire.h>
#include <INA226_WE.h>

// Variabili globali
extern bool accesa;
extern float tempo_accensione;
extern float tempo_totale;
extern float voltaggio_batteria;

// Funzioni
void setupVoltaggi();
void readVoltaggi();
void pubblicaVoltaggi(float voltaggio);
void pubblicaStatoVentola(bool stato);
