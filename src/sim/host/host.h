#pragma once
#include <stdint.h>
#include <string>
#include <vector>

// ═════════════════════════════════════════════════════════════════════════
//  O CONTROLE DO MUNDO FALSO
//
//  Quando o firmware roda no PC, quem manda no tempo e nos sensores é o
//  teste. É essa inversão que torna um teste de firmware possível: em vez
//  de esperar 20 segundos pelo aquecimento do ENS160, o teste ADIANTA o
//  relógio 20 segundos e segue.
//
//  Isto não é um truque de laboratório — é a prática normal em firmware.
//  Um teste que depende do tempo real é lento, é instável e, quando falha,
//  não dá para repetir.
// ═════════════════════════════════════════════════════════════════════════

// Volta tudo ao estado inicial: relógio em zero, log vazio, sensores
// falsos recém-ligados. Todo teste começa por aqui.
void host_reiniciar();

// Adianta o relógio. É o que millis() passa a devolver.
void host_avancar_ms(uint32_t ms);

// Adianta o relógio EM PASSOS, chamando loop() a cada passo. É assim que
// se roda "um minuto de firmware" em alguns milissegundos.
void host_rodar_loop(uint32_t total_ms, uint32_t passo_ms);

// Tudo o que o firmware imprimiu, uma linha por posição.
const std::vector<std::string>& host_log();

// Quantas linhas do log contêm este trecho.
int host_log_contem(const char* trecho);

// Ecoa no terminal o que o firmware imprime. Útil ao escrever um teste
// novo; atrapalha quando o teste já está pronto.
void host_ecoar(bool ligado);

// Faz millis() seguir o relógio de parede e delay() dormir de verdade.
// Usado só pelo  pio run -e native -t exec , que roda o firmware no PC em
// velocidade normal. Nos testes fica desligado.
void host_usar_relogio_real(bool ligado);
