#pragma once

// ═════════════════════════════════════════════════════════════════════════
//  Arduino.h — a versão de MENTIRA, para compilar o firmware no PC.
//
//  Quando o PlatformIO compila o ambiente 'native', este arquivo aparece
//  no lugar do <Arduino.h> de verdade. Ele oferece só o que este projeto
//  usa: o relógio, a serial, a UART e alguns tipos.
//
//  Repare no tamanho dele. Um firmware que precisasse de um shim de mil
//  linhas para compilar no PC estaria dizendo alguma coisa sobre a própria
//  arquitetura — que hardware e lógica estão embaralhados demais.
// ═════════════════════════════════════════════════════════════════════════

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Stream.h"

#ifndef PI
#define PI 3.1415926535897932384626433832795f
#endif

#define SERIAL_8N1 0x800001c

typedef uint8_t byte;

// O relógio. Nos testes, quem o move é host_avancar_ms().
unsigned long millis();
unsigned long micros();
void          delay(unsigned long ms);

// A biblioteca PMS usa isto para juntar os dois bytes de cada medida.
inline uint16_t makeWord(uint8_t alto, uint8_t baixo) {
    return (uint16_t)((alto << 8) | baixo);
}

// ── A serial do monitor ──────────────────────────────────────────────────
// Não escreve numa porta: guarda cada linha, para o teste poder afirmar
// coisas sobre o que o firmware disse. O log É a saída observável do
// firmware, no PC como na bancada.
class SerialFalsa {
public:
    void begin(unsigned long baud) { (void)baud; }
    void flush() {}
    operator bool() const { return true; }

    void print(const char* s);
    void print(int v);
    void print(float v);
    void println(const char* s);
    void println();
    int  printf(const char* formato, ...);
};

extern SerialFalsa Serial;

// ── A UART do sensor de partículas ───────────────────────────────────────
// Esta é um Stream de verdade: a biblioteca PMS REAL lê dela. O que muda é
// de onde vêm os bytes — em vez do sensor, de um quadro que o teste injeta.
//
// Ela também ESCUTA o que o firmware escreve: os comandos de acordar,
// dormir e modo ativo do PMS5003 são reconhecidos e mudam o estado do
// sensor falso, exatamente como no sensor de verdade.
class UartFalsa : public Stream {
public:
    void begin(unsigned long baud, uint32_t config = SERIAL_8N1,
               int8_t rx = -1, int8_t tx = -1);

    int    available() override;
    int    read() override;
    int    peek() override;
    size_t write(uint8_t b) override;
    size_t write(const uint8_t* dados, size_t n) override;

    // ── o que o teste pergunta ──
    bool foi_iniciada() const   { return _iniciada; }
    unsigned long baud() const  { return _baud; }
    int  pino_rx() const        { return _rx; }
    int  pino_tx() const        { return _tx; }
    bool esta_acordado() const  { return _acordado; }
    bool em_modo_ativo() const  { return _modo_ativo; }

    void limpar();
    void injetar(const uint8_t* dados, size_t n);

private:
    void interpretar_comando();

    bool           _iniciada   = false;
    unsigned long  _baud       = 0;
    int8_t         _rx         = -1;
    int8_t         _tx         = -1;
    bool           _acordado   = false;
    bool           _modo_ativo = false;

    uint8_t  _rx_buf[512];
    size_t   _rx_inicio = 0, _rx_fim = 0;
    uint8_t  _tx_buf[32];
    size_t   _tx_n = 0;
};

extern UartFalsa Serial2;
