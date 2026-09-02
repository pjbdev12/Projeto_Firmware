#pragma once
#include <stdint.h>

// ═════════════════════════════════════════════════════════════════════════
//  OS CONTROLES DOS SENSORES FALSOS
//
//  O teste liga e desliga o que quiser: o sensor que não responde, a
//  leitura que falha, o aquecimento que ainda não terminou. Nenhuma dessas
//  situações é fácil de provocar na bancada — algumas nem são possíveis —
//  e todas precisam estar cobertas, porque é nelas que o firmware erra.
//
//  Este arquivo não é incluído por nenhum código de firmware: só pelos
//  testes. O sensor_amb.cpp continua conhecendo apenas as bibliotecas.
// ═════════════════════════════════════════════════════════════════════════

void falsas_reiniciar();

// ── AHT21 ────────────────────────────────────────────────────────────────
void falso_aht_definir(float temperatura, float umidade);
void falso_aht_falhar_init(bool falhar);
void falso_aht_falhar_proxima_leitura(bool falhar);
bool falso_aht_foi_iniciado();

// ── ENS160 ───────────────────────────────────────────────────────────────
void falso_ens_falhar_init(bool falhar);
void falso_ens_definir(uint16_t eco2, uint16_t tvoc, uint8_t aqi);

// Quanto tempo o sensor passa se declarando "aquecendo" depois do begin().
// No sensor real são minutos; aqui é o que o teste quiser.
void falso_ens_aquecimento_ms(uint32_t ms);

bool    falso_ens_foi_iniciado();
uint8_t falso_ens_modo_configurado();      // o que setPWRMode() recebeu

// A compensação do TODO 4: o ENS160 precisa saber em que ar está medindo.
// Esquecer disso, no sensor real, só desvia o valor — ninguém percebe.
// Aqui fica registrado.
bool  falso_ens_foi_compensado();
float falso_ens_temperatura_recebida();
float falso_ens_umidade_recebida();

// ── PMS5003 ──────────────────────────────────────────────────────────────
// Monta um quadro de 32 bytes VÁLIDO (cabeçalho, campos e checksum) e o
// entrega à UART falsa, de onde a biblioteca PMS REAL vai lê-lo.
//
// Devolve false se o sensor falso ainda estiver dormindo ou fora do modo
// ativo — que é o que acontece de verdade quando falta o wakeUp() ou o
// activeMode() do TODO 6.
//
// As duas famílias de valores existem no mesmo quadro e são passadas
// separadamente de propósito: é assim que se descobre se o TODO 7 pegou a
// família certa (AE, ar ambiente) ou a errada (SP, laboratório).
bool falso_pms_injetar_quadro(uint16_t sp1_0, uint16_t sp2_5, uint16_t sp10,
                              uint16_t ae1_0, uint16_t ae2_5, uint16_t ae10);
