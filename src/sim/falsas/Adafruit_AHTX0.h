#pragma once
#include <Arduino.h>

// ═════════════════════════════════════════════════════════════════════════
//  Adafruit_AHTX0 — a versão de MENTIRA.
//
//  A API é a mesma da biblioteca real, byte por byte: mesmo nome de
//  classe, mesma assinatura, mesma CONVENÇÃO DE ERRO (begin() devolve
//  bool, true = respondeu). É por isso que o seu sensor_amb.cpp compila
//  contra ela sem alteração nenhuma.
//
//  Se esta API divergir da real, os testes passam e a placa falha. Por
//  isso ela é curta de propósito: só o que o projeto usa.
// ═════════════════════════════════════════════════════════════════════════

// A biblioteca real entrega as medidas neste formato padronizado, que ela
// usa para todos os sensores dela. Vem do Adafruit_Unified_Sensor.
typedef struct {
    float temperature;         // °C
    float relative_humidity;   // % UR
} sensors_event_t;

class Adafruit_AHTX0 {
public:
    bool begin();
    bool getEvent(sensors_event_t* umidade, sensors_event_t* temperatura);
};
