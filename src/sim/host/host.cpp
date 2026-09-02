#include "host.h"
#include "Arduino.h"
#include "Wire.h"

#include <chrono>
#include <thread>

// ═════════════════════════════════════════════════════════════════════════
//  O mundo falso: relógio, serial e UART.
// ═════════════════════════════════════════════════════════════════════════

SerialFalsa Serial;
UartFalsa   Serial2;
TwoWire     Wire;

static unsigned long            s_agora_ms   = 0;
static bool                     s_relogio_real = false;
static bool                     s_ecoar      = false;
static std::vector<std::string> s_log;
static std::string              s_parcial;   // linha ainda sem \n

static std::chrono::steady_clock::time_point s_inicio_real;


// ── O relógio ────────────────────────────────────────────────────────────

unsigned long millis() {
    if (s_relogio_real) {
        using namespace std::chrono;
        return (unsigned long)duration_cast<milliseconds>(
            steady_clock::now() - s_inicio_real).count();
    }
    return s_agora_ms;
}

unsigned long micros() { return millis() * 1000UL; }

void delay(unsigned long ms) {
    if (s_relogio_real) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    } else {
        s_agora_ms += ms;
    }
}


// ── Controles do teste ───────────────────────────────────────────────────

void host_avancar_ms(uint32_t ms) { s_agora_ms += ms; }

void host_ecoar(bool ligado) { s_ecoar = ligado; }

void host_usar_relogio_real(bool ligado) {
    s_relogio_real = ligado;
    s_inicio_real  = std::chrono::steady_clock::now();
}

const std::vector<std::string>& host_log() { return s_log; }

int host_log_contem(const char* trecho) {
    int n = 0;
    for (const auto& linha : s_log) {
        if (linha.find(trecho) != std::string::npos) {
            n++;
        }
    }
    return n;
}

// Declarada aqui para não obrigar quem inclui host.h a conhecer as falsas.
void falsas_reiniciar();

void host_reiniciar() {
    s_agora_ms = 0;
    s_log.clear();
    s_parcial.clear();
    Serial2.limpar();
    falsas_reiniciar();
}


// ── A serial: cada \n fecha uma linha do log ─────────────────────────────

static void acumular(const char* texto) {
    for (const char* p = texto; *p; p++) {
        if (*p == '\n') {
            if (s_ecoar) {
                // Sem o flush, a saída fica presa no buffer e some quando o
                // programa é interrompido — que é justamente como se sai
                // deste modo (Ctrl-C).
                fprintf(stdout, "%s\n", s_parcial.c_str());
                fflush(stdout);
            }
            s_log.push_back(s_parcial);
            s_parcial.clear();
        } else if (*p != '\r') {
            s_parcial.push_back(*p);
        }
    }
}

void SerialFalsa::print(const char* s) { acumular(s); }

void SerialFalsa::print(int v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", v);
    acumular(buf);
}

void SerialFalsa::print(float v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", v);
    acumular(buf);
}

void SerialFalsa::println(const char* s) { acumular(s); acumular("\n"); }
void SerialFalsa::println()              { acumular("\n"); }

int SerialFalsa::printf(const char* formato, ...) {
    char    buf[512];
    va_list args;
    va_start(args, formato);
    int n = vsnprintf(buf, sizeof(buf), formato, args);
    va_end(args);
    acumular(buf);
    return n;
}


// ═════════════════════════════════════════════════════════════════════════
//  A UART do PMS5003
//
//  Ela faz as duas pontas: entrega os bytes que o teste injetou, e escuta
//  os comandos que o firmware envia. É essa segunda metade que permite
//  verificar o TODO 6 — se o firmware não mandar wakeUp(), o sensor falso
//  continua dormindo e nenhum quadro aparece, igual ao de verdade.
// ═════════════════════════════════════════════════════════════════════════

void UartFalsa::begin(unsigned long baud, uint32_t config, int8_t rx, int8_t tx) {
    (void)config;
    _iniciada = true;
    _baud     = baud;
    _rx       = rx;
    _tx       = tx;
}

int UartFalsa::available() {
    return (int)(_rx_fim - _rx_inicio);
}

int UartFalsa::read() {
    if (_rx_inicio >= _rx_fim) {
        return -1;
    }
    return _rx_buf[_rx_inicio++];
}

int UartFalsa::peek() {
    return (_rx_inicio >= _rx_fim) ? -1 : _rx_buf[_rx_inicio];
}

size_t UartFalsa::write(uint8_t b) {
    if (_tx_n < sizeof(_tx_buf)) {
        _tx_buf[_tx_n++] = b;
    }
    interpretar_comando();
    return 1;
}

size_t UartFalsa::write(const uint8_t* dados, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (_tx_n < sizeof(_tx_buf)) {
            _tx_buf[_tx_n++] = dados[i];
        }
    }
    interpretar_comando();
    return n;
}

// Os comandos do PMS5003 têm 7 bytes e começam com 0x42 0x4D, como os
// quadros de dados. Estes são os que a biblioteca envia.
void UartFalsa::interpretar_comando() {
    while (_tx_n >= 7) {
        if (_tx_buf[0] != 0x42 || _tx_buf[1] != 0x4D) {
            memmove(_tx_buf, _tx_buf + 1, --_tx_n);   // dessincronizado
            continue;
        }

        const uint8_t tipo = _tx_buf[2];
        const uint8_t dado = _tx_buf[4];

        if (tipo == 0xE4) {
            _acordado = (dado == 0x01);          // wakeUp / sleep
        } else if (tipo == 0xE1) {
            _modo_ativo = (dado == 0x01);        // activeMode / passiveMode
        }

        memmove(_tx_buf, _tx_buf + 7, _tx_n - 7);
        _tx_n -= 7;
    }
}

void UartFalsa::limpar() {
    _rx_inicio = _rx_fim = 0;
    _tx_n      = 0;
    _iniciada  = false;
    _baud      = 0;
    _rx = _tx  = -1;
    _acordado  = false;
    _modo_ativo = false;
}

void UartFalsa::injetar(const uint8_t* dados, size_t n) {
    // Compacta o que já foi consumido antes de acrescentar.
    if (_rx_inicio > 0) {
        memmove(_rx_buf, _rx_buf + _rx_inicio, _rx_fim - _rx_inicio);
        _rx_fim   -= _rx_inicio;
        _rx_inicio = 0;
    }
    for (size_t i = 0; i < n && _rx_fim < sizeof(_rx_buf); i++) {
        _rx_buf[_rx_fim++] = dados[i];
    }
}
