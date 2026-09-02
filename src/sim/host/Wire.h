#pragma once
#include <stdint.h>

// O barramento I2C de mentira. As bibliotecas falsas dos sensores não
// falam com ele — recebem o ponteiro e o ignoram, que é o suficiente para
// o código do driver compilar e rodar sem alteração.
class TwoWire {
public:
    void begin() { _iniciado = true; }
    void begin(int sda, int scl) { (void)sda; (void)scl; _iniciado = true; }
    bool iniciado() const { return _iniciado; }
private:
    bool _iniciado = false;
};

extern TwoWire Wire;
