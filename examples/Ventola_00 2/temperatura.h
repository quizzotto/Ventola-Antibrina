#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>

/***************************
* Temperatura
****************************/
extern OneWire oneWire;
extern DallasTemperature s_temperatura;
extern float temperatura;
extern float temperatura_min;
extern unsigned long intervalloPubTemp;
void check_temperatura();
void startTempConversion();
void readTemperature();
void pubblicaTemperatura(float temp);
void pubblicaTemperaturaMinima(float tempMin);
