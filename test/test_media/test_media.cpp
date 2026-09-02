// ═════════════════════════════════════════════════════════════════════════
//  Testes de unidade da média móvel — rodam no SEU COMPUTADOR.
//
//      pio test -e native
//
//  Sem placa, sem cabo, sem sensor. Compila media.cpp com o compilador do
//  PC e verifica as quatro funções uma por uma.
//
//  Isto só é possível porque media.h não inclui <Arduino.h>: uma média
//  móvel é aritmética, e aritmética não precisa saber que existe um
//  microcontrolador. Guardar a lógica longe do hardware é o que permite
//  verificá-la em segundos em vez de em minutos de gravação.
//
//  Os testes NÃO dependem do valor de TAMANHO_HISTORICO. Você pode trocar
//  5, 10, 15 ou 30 no config.h — como pede o experimento da etapa — que
//  eles continuam valendo.
// ═════════════════════════════════════════════════════════════════════════

#include <unity.h>
#include <stdio.h>
#include <math.h>

#include "media.h"

#define N  TAMANHO_HISTORICO

// Tolerância das comparações de float. Somar 15 números em ponto flutuante
// não dá exatamente o mesmo resultado que a conta feita à mão — comparar
// float com == é uma das maneiras clássicas de escrever um teste que
// reprova código correto.
#define TOL  0.001f

static MediaMovel m;

void setUp(void)    { media_zerar(&m); }
void tearDown(void) { }


// ── O diagnóstico ────────────────────────────────────────────────────────
// O Unity escapa quebras de linha dentro das mensagens de erro, então uma
// explicação de seis linhas sairia toda amassada numa só. A saída vai por
// printf, antes do assert, e só quando o valor está errado.
static void explica(const char* texto) {
    printf("\n"
           "  ┌─ o que este teste esperava ─────────────────────────────────\n"
           "%s"
           "  └──────────────────────────────────────────────────────────────\n\n",
           texto);
}

#define VERIFICA_FLOAT(esperado, obtido, explicacao)                       \
    do {                                                                   \
        float _e = (float)(esperado), _o = (float)(obtido);                \
        if (fabsf(_e - _o) > TOL) { explica(explicacao); }                 \
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(TOL, _e, _o, "(veja acima)");     \
    } while (0)

#define VERIFICA_INT(esperado, obtido, explicacao)                         \
    do {                                                                   \
        int _e = (int)(esperado), _o = (int)(obtido);                      \
        if (_e != _o) { explica(explicacao); }                             \
        TEST_ASSERT_EQUAL_INT_MESSAGE(_e, _o, "(veja acima)");             \
    } while (0)

static char diag[2048];


// ── 1 ────────────────────────────────────────────────────────────────────
void test_comeca_vazia(void) {
    VERIFICA_INT(0, media_quantidade(&m),
        "  │ Depois de media_zerar(), a quantidade de leituras válidas\n"
        "  │ deveria ser 0. Confira se media_zerar() zera m->validas E\n"
        "  │ também m->indice.\n");

    VERIFICA_FLOAT(0.0f, media_valor(&m),
        "  │ A média de um histórico vazio deve ser 0.0 — e, principalmente,\n"
        "  │ não pode ser uma divisão por zero. É para isso que serve o\n"
        "  │ if (m->validas == 0) do TODO 2(a).\n");
}


// ── 2 ────────────────────────────────────────────────────────────────────
// O teste que pega o erro mais comum da etapa.
void test_uma_leitura_e_a_propria_media(void) {
    media_registrar(&m, 20.0f);

    VERIFICA_INT(1, media_quantidade(&m),
        "  │ Depois de UMA chamada a media_registrar(), deveria haver 1\n"
        "  │ leitura válida. Se deu 0, o TODO 1 ainda não incrementa\n"
        "  │ m->validas.\n");

    snprintf(diag, sizeof(diag),
        "  │ Registrei um único valor, 20.0, e a média saiu %.3f.\n"
        "  │\n"
        "  │ Se saiu 20/%d = %.3f, o TODO 2 está dividindo por\n"
        "  │ TAMANHO_HISTORICO em vez de dividir por m->validas. É o erro\n"
        "  │ clássico desta etapa: o histórico ainda está enchendo, e\n"
        "  │ dividir pelo tamanho total é o mesmo que inventar %d leituras\n"
        "  │ de valor zero que nunca existiram.\n"
        "  │\n"
        "  │ Se saiu 0.000, media_registrar() ainda não guarda nada\n"
        "  │ (TODO 1) ou media_valor() ainda devolve o placeholder 0.0f.\n",
        media_valor(&m), N, 20.0f / N, N - 1);

    VERIFICA_FLOAT(20.0f, media_valor(&m), diag);
}


// ── 3 ────────────────────────────────────────────────────────────────────
void test_media_de_tres_valores(void) {
    media_registrar(&m, 10.0f);
    media_registrar(&m, 20.0f);
    media_registrar(&m, 30.0f);

    VERIFICA_INT(3, media_quantidade(&m),
        "  │ Três chamadas a media_registrar() deveriam deixar 3 leituras\n"
        "  │ válidas no histórico.\n");

    VERIFICA_FLOAT(20.0f, media_valor(&m),
        "  │ (10 + 20 + 30) / 3 = 20.0\n"
        "  │\n"
        "  │ Se o resultado saiu menor, ou a soma está percorrendo posições\n"
        "  │ que ainda não foram preenchidas, ou a divisão está sendo feita\n"
        "  │ pelo tamanho do vetor em vez de por m->validas.\n");
}


// ── 4 ────────────────────────────────────────────────────────────────────
void test_o_contador_para_no_tamanho(void) {
    for (int i = 0; i < N * 3; i++) {
        media_registrar(&m, 7.0f);
    }

    snprintf(diag, sizeof(diag),
        "  │ Registrei %d valores num histórico de %d posições, e\n"
        "  │ media_quantidade() devolveu %d.\n"
        "  │\n"
        "  │ Depois que o vetor enche, ele não cresce mais: m->validas tem\n"
        "  │ de parar em TAMANHO_HISTORICO. Confira o\n"
        "  │     if (m->validas < TAMANHO_HISTORICO)\n"
        "  │ do TODO 1(c). Sem ele o contador passa do tamanho do vetor, e\n"
        "  │ a soma do TODO 2 começa a ler memória que não lhe pertence.\n",
        N * 3, N, media_quantidade(&m));

    VERIFICA_INT(N, media_quantidade(&m), diag);

    VERIFICA_FLOAT(7.0f, media_valor(&m),
        "  │ Todos os valores registrados foram 7.0, então a média tem de\n"
        "  │ ser 7.0 — não importa quantos couberam.\n");
}


// ── 5 ────────────────────────────────────────────────────────────────────
// O teste do vetor circular: a leitura mais antiga tem de SAIR.
void test_a_mais_antiga_e_descartada(void) {
    for (int i = 0; i < N; i++) {
        media_registrar(&m, 10.0f);      // enche tudo com 10
    }
    media_registrar(&m, 100.0f);         // uma a mais: substitui a primeira

    float esperado = (100.0f + 10.0f * (N - 1)) / N;

    snprintf(diag, sizeof(diag),
        "  │ Enchi o histórico com %d valores 10.0 e registrei mais um,\n"
        "  │ 100.0. Esse 100.0 deveria ter substituído o 10.0 mais antigo,\n"
        "  │ deixando a média em (100 + 10*%d) / %d = %.3f.\n"
        "  │ Saiu %.3f.\n"
        "  │\n"
        "  │ Se saiu 10.000, o valor novo não foi gravado: o índice passou\n"
        "  │ do fim do vetor em vez de voltar ao começo. É isso que o\n"
        "  │ operador %% do TODO 1(b) faz:\n"
        "  │     m->indice = (m->indice + 1) %% TAMANHO_HISTORICO;\n"
        "  │\n"
        "  │ E repare: escrever fora do vetor não dá erro na hora. Dá erro\n"
        "  │ depois, em outro lugar, de um jeito que não parece ter relação\n"
        "  │ nenhuma com a média móvel.\n",
        N, N - 1, N, esperado, media_valor(&m));

    VERIFICA_FLOAT(esperado, media_valor(&m), diag);
}


// ── 6 ────────────────────────────────────────────────────────────────────
// A razão de ser do struct: seis grandezas, um tipo, nenhuma interferência.
void test_instancias_sao_independentes(void) {
    MediaMovel temperatura, umidade;
    media_zerar(&temperatura);
    media_zerar(&umidade);

    media_registrar(&temperatura, 25.0f);
    media_registrar(&umidade,     60.0f);
    media_registrar(&umidade,     70.0f);

    VERIFICA_FLOAT(25.0f, media_valor(&temperatura),
        "  │ Duas variáveis MediaMovel diferentes não podem se misturar.\n"
        "  │ Registrei 25.0 numa e 60.0/70.0 na outra, e a primeira mudou.\n"
        "  │\n"
        "  │ Se este teste falhou, alguma coisa em media.cpp está guardada\n"
        "  │ numa variável 'static' de arquivo em vez de estar dentro do\n"
        "  │ struct — e aí as seis grandezas do projeto dividiriam o mesmo\n"
        "  │ histórico. É exatamente o que o struct existe para evitar.\n");

    VERIFICA_FLOAT(65.0f, media_valor(&umidade),
        "  │ (60 + 70) / 2 = 65.0\n");

    VERIFICA_INT(1, media_quantidade(&temperatura), "  │ Uma leitura registrada.\n");
    VERIFICA_INT(2, media_quantidade(&umidade),     "  │ Duas leituras registradas.\n");
}


// ── 7 ────────────────────────────────────────────────────────────────────
// media_zerar() é chamada no começo de cada ciclo de coleta (Etapa 3).
void test_zerar_no_meio_do_uso(void) {
    for (int i = 0; i < N; i++) {
        media_registrar(&m, 50.0f);
    }

    media_zerar(&m);

    VERIFICA_INT(0, media_quantidade(&m),
        "  │ Depois de media_zerar(), o histórico tem de estar vazio de\n"
        "  │ novo — mesmo que estivesse cheio. A partir da Etapa 3 isso\n"
        "  │ acontece a cada ciclo de coleta, então precisa funcionar mais\n"
        "  │ de uma vez, e não só no setup().\n");

    media_registrar(&m, 1.0f);

    VERIFICA_FLOAT(1.0f, media_valor(&m),
        "  │ A primeira leitura depois de um media_zerar() é, sozinha, a\n"
        "  │ média. Se apareceram valores antigos misturados, media_zerar()\n"
        "  │ não está reiniciando m->indice — a leitura nova foi parar no\n"
        "  │ meio do vetor e as velhas continuaram dentro da conta.\n");
}


// ═════════════════════════════════════════════════════════════════════════
int main(int argc, char** argv) {
    (void)argc; (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_comeca_vazia);
    RUN_TEST(test_uma_leitura_e_a_propria_media);
    RUN_TEST(test_media_de_tres_valores);
    RUN_TEST(test_o_contador_para_no_tamanho);
    RUN_TEST(test_a_mais_antiga_e_descartada);
    RUN_TEST(test_instancias_sao_independentes);
    RUN_TEST(test_zerar_no_meio_do_uso);

    // Devolver 0 sempre: o PlatformIO interpreta um codigo de saida diferente
    // de zero como "o programa morreu", e o numero de falhas viraria o nome
    // de um sinal ("SIGABRT"). Quem decide se a suite passou e o proprio
    // PlatformIO, lendo as linhas de PASSED/FAILED acima.
    UNITY_END();
    return 0;
}
