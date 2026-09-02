#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────
// Sensores de ambiente, os dois no mesmo barramento I2C:
//
//   AHT21  (0x38) → temperatura e umidade relativa
//   ENS160 (0x53) → eCO2, TVOC e um índice de qualidade do ar (AQI)
//
// Eles não são independentes: a resposta do ENS160 depende da temperatura
// e da umidade do ar. Por isso, a cada leitura, o valor do AHT21 é
// entregue ao ENS160 para compensação. É o motivo de os dois estarem no
// mesmo módulo em vez de em dois separados.
// ─────────────────────────────────────────────────────────────────────────

typedef struct {
    float    temperatura;    // °C
    float    umidade;        // % de umidade relativa
    uint16_t eco2;           // ppm  (equivalente de CO2)
    uint16_t tvoc;           // ppb  (compostos orgânicos voláteis totais)
    uint8_t  aqi;            // 1 (excelente) a 5 (insalubre)

    // O ENS160 informa quando ainda não confia na própria saída: ele passa
    // por um aquecimento de alguns minutos ao ser ligado. Enquanto este
    // campo for false, os três valores de gás acima existem mas não valem —
    // e não devem entrar em nenhuma média.
    bool     gases_validos;

    // A leitura como um todo deu certo? Se for false, NADA aqui dentro
    // vale — nem a temperatura. Repare que a estrutura carrega a própria
    // validade, em vez de deixá-la num valor de retorno separado: assim o
    // dado e o "pode confiar nele?" andam sempre juntos.
    bool     ok;
} LeituraAmbiente;

// Liga o barramento I2C e prepara os dois sensores.
// Retorna false se qualquer um deles não responder.
bool amb_init();

// Faz uma leitura completa e DEVOLVE a estrutura preenchida:
//
//     LeituraAmbiente amb = amb_ler();
//     if (amb.ok) { ... }
//
// Se a leitura falhar, o campo .ok volta false e todo o resto volta zerado.
LeituraAmbiente amb_ler();

// Texto legível para o estado do ENS160, para o log fazer sentido.
const char* amb_texto_status();

// Texto legível para o índice de qualidade do ar (1 a 5).
const char* amb_texto_aqi(uint8_t aqi);
