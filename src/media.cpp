#include "media.h"

// ═════════════════════════════════════════════════════════════════════════
//  Média móvel
//
//  A ideia: em vez de mostrar a última leitura (que pula para todo lado), 
//  guardamos as N últimas em um vetor e mostramos a média
//  delas. Uma leitura ruim isolada perde força; uma mudança de verdade,
//  que se repete, aparece do mesmo jeito — só que com um pequeno atraso.
//
//  O vetor é CIRCULAR: quando chega no fim, o índice volta ao começo e
//  sobrescreve a leitura mais antiga. Assim nunca precisamos "empurrar"
//  todos os valores para o lado.
//
//      m->historico:  [ 21.4 ][ 21.6 ][ 21.5 ][ ---- ][ ---- ]
//                                                ↑
//                                            m->indice
//
//  Repare que TODAS as funções aqui recebem  MediaMovel* m : elas não
//  sabem de qual grandeza estão cuidando, e é justamente por isso que as
//  mesmas quatro funções servem para as seis medidas do projeto.
// ═════════════════════════════════════════════════════════════════════════

void media_zerar(MediaMovel* m) {
    m->indice  = 0;
    m->validas = 0;
}


void media_registrar(MediaMovel* m, float valor) {
    // ─────────────────────────────────────────────────────────────────────
    // TODO 1 — Guardar o valor e avançar o índice em círculo.
    //
    m->historico[m->indice] = valor;
    m->indice = (m->indice+1)%TAMANHO_HISTORICO;
    if (m->validas < TAMANHO_HISTORICO)
    {
        m->validas = m->validas + 1;
    }
    //   a) grave o valor:  m->historico[m->indice] = valor;
    //      (a seta  ->  é como se acessa um campo de struct por ponteiro;
    //       equivale a  (*m).historico[...] , só que legível — cap. 10)
    //
    //   b) avance o índice dando a volta ao chegar no fim. O operador %
    //      (resto da divisão) faz exatamente isso:
    //          m->indice = (m->indice + 1) % TAMANHO_HISTORICO;
    //      Com TAMANHO_HISTORICO = 5, os índices viram 0,1,2,3,4,0,1,2,...
    //
    //   c) enquanto  m->validas < TAMANHO_HISTORICO, incremente m->validas.
    //      (depois que o vetor encheu, ele não cresce mais)
    // ─────────────────────────────────────────────────────────────────────
}


float media_valor(const MediaMovel* m) {
    // ─────────────────────────────────────────────────────────────────────
    // TODO 2 — Calcular a média SÓ das posições já preenchidas.
    if (m->validas == 0){
            return 0.0f;
    }
    float soma = 0.0f;
    for (int i = 0; i < m->validas; i++) {
        soma += m->historico[i];
    }
    return soma / m->validas;
    //
    //   a) se  m->validas == 0, devolva 0.0f — sem isso, a divisão do
    //      item (c) seria uma divisão por zero na primeira chamada;
    //
    //   b) some  m->historico[0] .. m->historico[m->validas - 1]
    //      em uma variável  float soma = 0.0f;
    //
    //   c) devolva  soma / m->validas;
    //
    // Por que dividir por validas e não por TAMANHO_HISTORICO? Porque no
    // começo o vetor ainda está enchendo. Dividir pelo tamanho total faria
    // as primeiras médias saírem artificialmente baixas — como se houvesse
    // leituras de valor zero que nunca existiram.
    // ─────────────────────────────────────────────────────────────────────
    //return 0.0f;   // ← placeholder: troque pelo cálculo do TODO 2
}


int media_quantidade(const MediaMovel* m) {
    return m->validas;
}
