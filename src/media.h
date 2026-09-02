#pragma once
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────
// Média móvel reutilizável.
//
// Este projeto acompanha SEIS grandezas (temperatura, umidade, eCO2, TVOC,
// PM2.5 e PM10). Escrever seis vetores, seis índices e seis contadores
// espalhados seria seis vezes a mesma chance de errar.
//
// Em vez disso, o histórico e seus contadores viram um struct (cap. 10):
// um tipo que empacota "tudo o que uma média móvel precisa saber". Cada
// grandeza ganha a sua própria variável desse tipo, e as funções abaixo
// recebem, por ponteiro, qual delas devem manipular.
// ─────────────────────────────────────────────────────────────────────────

typedef struct {
    float historico[TAMANHO_HISTORICO];
    int   indice;    // onde a próxima leitura será gravada
    int   validas;   // quantas posições já contêm dados reais
} MediaMovel;

// Esvazia o histórico (usado no início de cada ciclo de coleta).
void  media_zerar(MediaMovel* m);

// Guarda mais uma leitura, descartando a mais antiga.
void  media_registrar(MediaMovel* m, float valor);

// Média das leituras já guardadas. Devolve 0 se ainda não houver nenhuma.
float media_valor(const MediaMovel* m);

// Quantas leituras válidas há no histórico agora.
int   media_quantidade(const MediaMovel* m);
