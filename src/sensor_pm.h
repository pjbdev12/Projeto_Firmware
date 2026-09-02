#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────
// PMS5003 — material particulado, pela UART.
//
// Diferente dos sensores I2C, este não responde a perguntas: uma vez
// acordado, ele simplesmente ENVIA um quadro de 32 bytes de tempos em
// tempos, por conta própria. Cabe ao nosso lado escutar.
//
// Ele também tem uma ventoinha, que puxa o ar através de uma câmara óptica.
// Isso traz duas consequências que nenhum sensor I2C deste projeto tem:
// consumo alto (~100 mA) e um tempo físico de estabilização do fluxo de ar
// antes de a medida corresponder ao ambiente.
// ─────────────────────────────────────────────────────────────────────────

typedef struct {
    uint16_t pm1_0;   // µg/m³ — partículas de até 1,0 µm
    uint16_t pm2_5;   // µg/m³ — as que penetram nos alvéolos: a medida principal
    uint16_t pm10;    // µg/m³ — até 10 µm

    // Esta leitura vale? Ela é false em duas situações, e nenhuma das duas
    // é erro: quando ainda não chegou um quadro completo (o sensor manda um
    // por segundo, e o laço gira muito mais rápido que isso) e quando a
    // ventoinha ainda não girou o bastante desde que foi ligada.
    bool     ok;
} LeituraParticulas;

// Abre a UART e acorda o sensor.
bool pm_init();

// Consome o que chegou pela UART e DEVOLVE a estrutura:
//
//     LeituraParticulas pm = pm_ler();
//     if (pm.ok) { ... }
//
// Não bloqueia: se não chegou quadro completo, volta com .ok = false na
// hora, sem esperar por nada.
LeituraParticulas pm_ler();

// Desliga a ventoinha (o grosso do consumo do dispositivo).
void pm_dormir();

// Religa a ventoinha, e reinicia a contagem do aquecimento: as leituras
// voltam a valer só depois de PM_AQUECIMENTO_MS.
void pm_acordar();

// Quanto tempo a ventoinha precisa girar antes de a leitura valer.
//
// É a mesma ideia do aquecimento do ENS160, com uma diferença que muda tudo:
// o ENS160 avisa sozinho que ainda não confia em si mesmo, e basta perguntar.
// O PMS5003 não avisa nada — ele entrega um número com toda a confiança do
// mundo enquanto o ar dentro da câmara ainda é o de antes. Quem tem de
// lembrar é o firmware.
//
// O valor inicial vem de PM_AQUECIMENTO_MS, no config.h. Esta função existe
// para os testes exercitarem o aquecimento sem precisar recompilar o projeto
// inteiro com outro valor.
void pm_definir_aquecimento_ms(uint32_t ms);
