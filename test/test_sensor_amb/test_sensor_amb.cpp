// ═════════════════════════════════════════════════════════════════════════
//  Testes do driver dos sensores I²C — TODO 3, 4 e 5.
//
//      pio test -e native -v
//
//  O sensor_amb.cpp que roda aqui é o MESMO arquivo que vai para a placa,
//  sem uma linha alterada. O que muda é com quem ele conversa: em vez das
//  bibliotecas da Adafruit e da DFRobot, um par de bibliotecas falsas que
//  respondem ao teste.
//
//  Isso permite provocar coisas que a bancada não provoca:
//
//    · o sensor que não responde       (sem soltar fio nenhum)
//    · a leitura que falha             (sem sensor defeituoso)
//    · o aquecimento do ENS160         (20 s em vez de minutos, e a cada teste)
//
//  A última é a mais importante. No sensor real o aquecimento acontece uma
//  vez, dura minutos e você não consegue repeti-lo sem desligar tudo — e é
//  justamente durante ele que o firmware toma a decisão que mais erra.
// ═════════════════════════════════════════════════════════════════════════

#include <unity.h>

#include "sensor_amb.h"
#include "config.h"

#include "host.h"
#include "falsas.h"
#include "DFRobot_ENS160.h"     // para ENS160_STANDARD_MODE

static void explica(const char* texto) {
    printf("\n"
           "  ┌─ o que este teste esperava ─────────────────────────────────\n"
           "%s"
           "  └──────────────────────────────────────────────────────────────\n\n",
           texto);
}

// A condição é avaliada UMA vez e guardada. Escrever a macro com
// (condicao) nos dois lugares faria a chamada acontecer duas vezes — e uma
// função como pm_ler(), que consome o buffer, devolveria coisas diferentes
// em cada uma. Efeito colateral dentro de macro é armadilha clássica de C,
// e um teste também é código.
#define VERIFICA(condicao, explicacao)                                     \
    do {                                                                   \
        const bool resultado = (condicao);                                 \
        if (!resultado) { explica(explicacao); }                           \
        TEST_ASSERT_TRUE_MESSAGE(resultado, "(veja acima)");               \
    } while (0)

void setUp(void)    { host_reiniciar(); }
void tearDown(void) { }


// ═════════════════════════════════════════════════════════════════════════
//  TODO 3 — inicializar os dois e verificar CADA UM
// ═════════════════════════════════════════════════════════════════════════

void test_init_da_certo_quando_os_dois_respondem(void) {
    VERIFICA(amb_init() == true,
        "  │ Com os dois sensores respondendo, amb_init() tem de devolver\n"
        "  │ true.\n"
        "  │\n"
        "  │ Se devolveu false, o teste do ENS160 provavelmente está\n"
        "  │ invertido: essa biblioteca devolve int, e sucesso é ZERO\n"
        "  │ (NO_ERR). Escrever  if (s_ens160.begin())  dispara o erro\n"
        "  │ justamente quando deu certo.\n");

    VERIFICA(falso_aht_foi_iniciado(),
        "  │ amb_init() nunca chamou s_aht.begin(). O TODO 3 pede que os\n"
        "  │ DOIS sensores sejam inicializados.\n");

    VERIFICA(falso_ens_foi_iniciado(),
        "  │ amb_init() nunca chamou s_ens160.begin().\n");
}


void test_init_falha_quando_o_aht_nao_responde(void) {
    falso_aht_falhar_init(true);

    VERIFICA(amb_init() == false,
        "  │ O AHT21 não respondeu, e amb_init() devolveu true.\n"
        "  │\n"
        "  │ Sem essa verificação o programa segue lendo um sensor que não\n"
        "  │ está lá, e o erro só aparece muito depois — como números\n"
        "  │ estranhos, não como uma mensagem de falha.\n"
        "  │\n"
        "  │ Esta biblioteca devolve bool: false = não respondeu.\n"
        "  │     if (!s_aht.begin()) { ...; return false; }\n");
}


void test_init_falha_quando_o_ens_nao_responde(void) {
    falso_ens_falhar_init(true);

    VERIFICA(amb_init() == false,
        "  │ O ENS160 não respondeu, e amb_init() devolveu true.\n"
        "  │\n"
        "  │ ATENÇÃO à convenção: esta biblioteca devolve int, e o sucesso\n"
        "  │ é ZERO. O teste certo é\n"
        "  │     if (s_ens160.begin() != NO_ERR) { ...; return false; }\n"
        "  │\n"
        "  │ Se você escreveu  if (s_ens160.begin())  , o programa compila,\n"
        "  │ roda, e faz o contrário do que você quis — sem nenhum aviso.\n"
        "  │ Repare que este teste e o anterior, juntos, prendem os dois\n"
        "  │ lados: um sensor que falha tem de reprovar, e dois sensores\n"
        "  │ que respondem têm de aprovar.\n");
}


void test_init_poe_o_ens_em_medicao_continua(void) {
    amb_init();

    VERIFICA(falso_ens_modo_configurado() == ENS160_STANDARD_MODE,
        "  │ Falta  s_ens160.setPWRMode(ENS160_STANDARD_MODE);  no TODO 3.\n"
        "  │\n"
        "  │ Sem isso o sensor fica no modo em que acordou, e pode não\n"
        "  │ estar medindo continuamente — os valores de gás saem parados\n"
        "  │ ou desatualizados, sem nenhuma indicação de erro.\n");
}


// ═════════════════════════════════════════════════════════════════════════
//  A leitura
// ═════════════════════════════════════════════════════════════════════════

void test_leitura_traz_temperatura_e_umidade(void) {
    falso_aht_definir(21.5f, 62.0f);
    amb_init();

    LeituraAmbiente leitura = amb_ler();
    VERIFICA(leitura.ok == true,
        "  │ amb_ler() voltou com .ok = false, com o sensor respondendo\n"
        "  │ normalmente.\n");

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 21.5f, leitura.temperatura,
        "a temperatura lida nao e a que o sensor reportou");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 62.0f, leitura.umidade,
        "a umidade lida nao e a que o sensor reportou");
}


void test_leitura_que_falha_devolve_false(void) {
    amb_init();
    falso_aht_falhar_proxima_leitura(true);

    LeituraAmbiente leitura = amb_ler();
    VERIFICA(leitura.ok == false,
        "  │ O AHT21 falhou e amb_ler() voltou com .ok = true.\n"
        "  │\n"
        "  │ Quando a leitura falha, nada na estrutura vale — e quem chamou\n"
        "  │ precisa saber disso para descartar a amostra em vez de\n"
        "  │ registrá-la na média.\n");
}


// ═════════════════════════════════════════════════════════════════════════
//  TODO 4 — a compensação
// ═════════════════════════════════════════════════════════════════════════

void test_leitura_compensa_o_ens160(void) {
    falso_aht_definir(28.0f, 41.0f);
    amb_init();

    LeituraAmbiente leitura = amb_ler();

    VERIFICA(falso_ens_foi_compensado(),
        "  │ amb_ler() nunca chamou s_ens160.setTempAndHum().\n"
        "  │\n"
        "  │ O ENS160 é um sensor de óxido metálico: a resistência do\n"
        "  │ elemento dele muda com a temperatura e com a umidade, não só\n"
        "  │ com os gases. Sem informar as duas, o eCO2 passa a variar com\n"
        "  │ o clima do dia em vez de variar com o ar da sala.\n"
        "  │\n"
        "  │ No sensor de verdade esse erro é quase invisível: o valor só\n"
        "  │ fica um pouco deslocado, e ninguém desconfia. É por isso que\n"
        "  │ ele merece um teste.\n"
        "  │\n"
        "  │     s_ens160.setTempAndHum(r.temperatura, r.umidade);\n");

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 28.0f, falso_ens_temperatura_recebida(),
        "o ENS160 recebeu uma temperatura diferente da que foi lida");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 41.0f, falso_ens_umidade_recebida(),
        "o ENS160 recebeu uma umidade diferente da que foi lida "
        "(os dois argumentos podem estar trocados de lugar)");
}


// ═════════════════════════════════════════════════════════════════════════
//  TODO 5 — o estado do sensor, que é o que o firmware precisa respeitar
// ═════════════════════════════════════════════════════════════════════════

void test_gases_nao_valem_durante_o_aquecimento(void) {
    falso_ens_aquecimento_ms(20000);
    amb_init();

    host_avancar_ms(5000);           // ainda dentro do aquecimento
    LeituraAmbiente leitura = amb_ler();

    VERIFICA(leitura.gases_validos == false,
        "  │ O ENS160 ainda estava aquecendo e gases_validos saiu true.\n"
        "  │\n"
        "  │ O sensor informa o próprio estado, e o firmware precisa\n"
        "  │ registrar isso:\n"
        "  │     s_status = s_ens160.getENS160Status();\n"
        "  │     r.gases_validos = (s_status == ENS160_OPERACAO_NORMAL);\n");
}


void test_gases_valem_depois_do_aquecimento(void) {
    falso_ens_aquecimento_ms(20000);
    falso_ens_definir(780, 150, 3);
    amb_init();

    host_avancar_ms(25000);          // o aquecimento terminou
    LeituraAmbiente leitura = amb_ler();

    VERIFICA(leitura.gases_validos == true,
        "  │ O aquecimento acabou, o sensor reporta 'operacao normal', e\n"
        "  │ gases_validos continuou false.\n"
        "  │\n"
        "  │ Se este teste falha e o anterior passa, provavelmente o campo\n"
        "  │ ainda é o placeholder  r.gases_validos = false;  do\n"
        "  │ esqueleto: ele acerta por acaso enquanto o sensor aquece, e\n"
        "  │ erra para sempre depois.\n");

    VERIFICA(leitura.eco2 == 780 && leitura.tvoc == 150 && leitura.aqi == 3,
        "  │ Os três valores de gás não chegaram à estrutura de saída.\n"
        "  │ O TODO 5 pede as três leituras:\n"
        "  │     r.aqi  = s_ens160.getAQI();\n"
        "  │     r.tvoc = s_ens160.getTVOC();\n"
        "  │     r.eco2 = s_ens160.getECO2();\n");
}


void test_o_texto_de_estado_acompanha_o_sensor(void) {
    falso_ens_aquecimento_ms(20000);
    amb_init();

    LeituraAmbiente leitura = amb_ler();
    const bool aquecendo_ok = (strcmp(amb_texto_status(), "aquecendo") == 0);

    host_avancar_ms(25000);
    leitura = amb_ler();
    const bool normal_ok = (strcmp(amb_texto_status(), "operacao normal") == 0);

    VERIFICA(aquecendo_ok && normal_ok,
        "  │ amb_texto_status() não acompanhou o estado do sensor.\n"
        "  │\n"
        "  │ Esse texto sai no fim de cada linha do monitor, e é por ele\n"
        "  │ que se descobre, na bancada, que o eCO2 está preso em 400\n"
        "  │ porque o sensor ainda está aquecendo. Ele depende de o TODO 5\n"
        "  │ guardar o estado em s_status.\n");
}


// ═════════════════════════════════════════════════════════════════════════
int main(int argc, char** argv) {
    (void)argc; (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_init_da_certo_quando_os_dois_respondem);
    RUN_TEST(test_init_falha_quando_o_aht_nao_responde);
    RUN_TEST(test_init_falha_quando_o_ens_nao_responde);
    RUN_TEST(test_init_poe_o_ens_em_medicao_continua);

    RUN_TEST(test_leitura_traz_temperatura_e_umidade);
    RUN_TEST(test_leitura_que_falha_devolve_false);
    RUN_TEST(test_leitura_compensa_o_ens160);

    RUN_TEST(test_gases_nao_valem_durante_o_aquecimento);
    RUN_TEST(test_gases_valem_depois_do_aquecimento);
    RUN_TEST(test_o_texto_de_estado_acompanha_o_sensor);

    // Devolver 0 sempre: o PlatformIO interpreta um codigo de saida diferente
    // de zero como "o programa morreu", e o numero de falhas viraria o nome
    // de um sinal ("SIGABRT"). Quem decide se a suite passou e o proprio
    // PlatformIO, lendo as linhas de PASSED/FAILED acima.
    UNITY_END();
    return 0;
}
