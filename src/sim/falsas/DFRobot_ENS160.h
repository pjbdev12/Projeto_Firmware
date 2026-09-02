#pragma once
#include <Arduino.h>
#include <Wire.h>

// ═════════════════════════════════════════════════════════════════════════
//  DFRobot_ENS160 — a versão de MENTIRA.
//
//  Repare na convenção de erro, que é o OPOSTO da do AHT21: begin()
//  devolve int, e sucesso é ZERO. Escrever  if (s_ens160.begin())
//  "funciona" e está errado — o if dispara justamente quando deu certo.
//
//  A falsa reproduz isso porque é o ponto do TODO 3. Um fake que
//  "simplificasse" para bool esconderia exatamente o erro que o teste
//  existe para pegar.
// ═════════════════════════════════════════════════════════════════════════

#define NO_ERR                  0
#define ERR_DATA_BUS          (-1)
#define ERR_IC_VERSION        (-2)

#define ENS160_SLEEP_MODE       0x00
#define ENS160_IDLE_MODE        0x01
#define ENS160_STANDARD_MODE    0x02

class DFRobot_ENS160_I2C {
public:
    DFRobot_ENS160_I2C(TwoWire* barramento, uint8_t endereco)
        : _barramento(barramento), _endereco(endereco) {}

    int      begin();
    void     setPWRMode(uint8_t modo);
    void     setTempAndHum(float temperatura, float umidade);

    uint8_t  getENS160Status();
    uint8_t  getAQI();
    uint16_t getTVOC();
    uint16_t getECO2();

private:
    TwoWire* _barramento;
    uint8_t  _endereco;
};
