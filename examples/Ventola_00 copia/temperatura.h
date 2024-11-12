#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>

/***************************
* Temperatura
****************************/
// Dichiarazione delle variabili (definite in temperatura.cpp)
extern OneWire oneWire;
extern DallasTemperature s_temperatura;
extern float temperatura;
extern float temperatura_min;
extern unsigned long intervalloPubTemp;

// Funzioni pubbliche
void setupTemperature();
void readTemperature();
void pubblicaTemperatura(float temp);
void pubblicaTemperaturaMinima(float tempMin);
