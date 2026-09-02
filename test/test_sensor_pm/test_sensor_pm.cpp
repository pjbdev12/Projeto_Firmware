// ═════════════════════════════════════════════════════════════════════════
//  Testes do driver do PMS5003 — TODO 6 e 7.
//
//      pio test -e native -v
//
//  Aqui há uma diferença em relação aos sensores I²C, e ela é o motivo de
//  este arquivo existir: a biblioteca do PMS5003 usada nestes testes é a
//  REAL, a mesma que vai para a placa. Ela só precisa de um Stream e de
//  millis() — não toca em registrador nenhum —, então roda no PC sem
//  alteração.
//
//  O que é falso é o outro lado do fio: uma UART que entrega os bytes que
//  o teste injeta. Os quadros injetados têm 32 bytes, cabeçalho e checksum
//  válidos, exatamente como os do sensor. Quem os interpreta é o parser de
//  verdade.
//
//  E a UART falsa escuta o que o firmware ESCREVE: os comandos de acordar
//  e de modo ativo são reconhecidos. Se o TODO 6 esquecer o wakeUp(), o
//  sensor falso continua dormindo e nenhum quadro chega — o mesmo silêncio
//  da bancada, pelo mesmo motivo.
// ═════════════════════════════════════════════════════════════════════════

#include <unity.h>

#include "sensor_pm.h"
#include "config.h"

#include "host.h"
#include "falsas.h"

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

void setUp(void) {
    host_reiniciar();
    // O aquecimento é estado do módulo e sobrevive de um teste para o
    // outro. Cada teste começa do valor do config.h e muda se precisar.
    pm_definir_aquecimento_ms(PM_AQUECIMENTO_MS);
}
void tearDown(void) { }

static const char* AVISO_UART_FECHADA =
    "  │ A UART nunca foi aberta: falta a primeira linha do TODO 6,\n"
    "  │     Serial2.begin(PMS_BAUD, SERIAL_8N1, PMS_PINO_RX, PMS_PINO_TX);\n"
    "  │\n"
    "  │ Sem ela nenhum byte chega, e o sintoma na bancada é o mesmo de um\n"
    "  │ fio trocado: silêncio absoluto, sem erro nenhum.\n";


// ═════════════════════════════════════════════════════════════════════════
//  TODO 6 — abrir a porta e acordar o sensor
// ═════════════════════════════════════════════════════════════════════════

void test_init_abre_a_uart(void) {
    pm_init();

    VERIFICA(Serial2.foi_iniciada(), AVISO_UART_FECHADA);

    VERIFICA(Serial2.baud() == PMS_BAUD,
        "  │ A UART foi aberta num baud diferente de PMS_BAUD (9600).\n"
        "  │\n"
        "  │ 9600 é fixo no PMS5003; não é uma escolha nossa. Num baud\n"
        "  │ errado os bytes chegam, mas embaralhados — e o parser\n"
        "  │ simplesmente nunca fecha um quadro.\n");
}


void test_init_usa_os_pinos_do_config(void) {
    pm_init();

    VERIFICA(Serial2.pino_rx() == PMS_PINO_RX && Serial2.pino_tx() == PMS_PINO_TX,
        "  │ Os pinos passados a Serial2.begin() não são os do config.h,\n"
        "  │ ou estão trocados entre si.\n"
        "  │\n"
        "  │ A ordem dos dois últimos argumentos é (RX, TX) — nessa ordem:\n"
        "  │     Serial2.begin(PMS_BAUD, SERIAL_8N1, PMS_PINO_RX, PMS_PINO_TX);\n"
        "  │\n"
        "  │ Trocá-los aqui produz o mesmo silêncio que trocar os fios, e é\n"
        "  │ ainda mais difícil de achar, porque a fiação está certa.\n");
}


void test_init_acorda_a_ventoinha(void) {
    pm_init();

    VERIFICA(Serial2.esta_acordado(),
        "  │ O sensor não foi acordado: falta  s_pms.wakeUp();  no TODO 6.\n"
        "  │\n"
        "  │ O PMS5003 pode estar dormindo quando o programa começa, e um\n"
        "  │ sensor dormindo não manda quadro nenhum. Na bancada isso se\n"
        "  │ parece com fio solto.\n");
}


void test_init_poe_em_modo_ativo(void) {
    pm_init();

    VERIFICA(Serial2.em_modo_ativo(),
        "  │ Falta  s_pms.activeMode();  no TODO 6.\n"
        "  │\n"
        "  │ No modo ativo o sensor manda um quadro por conta própria, mais\n"
        "  │ ou menos uma vez por segundo. A alternativa (passiveMode) exige\n"
        "  │ pedir e ESPERAR a resposta — o que bloquearia o laço, que é\n"
        "  │ justamente o que este projeto evita.\n");
}


// ═════════════════════════════════════════════════════════════════════════
//  A leitura
// ═════════════════════════════════════════════════════════════════════════

void test_sem_quadro_nao_ha_leitura(void) {
    pm_init();

    LeituraParticulas pm = pm_ler();
    VERIFICA(pm.ok == false,
        "  │ Nada chegou pela UART e pm_ler() voltou com .ok = true.\n"
        "  │\n"
        "  │ Devolver true sem quadro faria o programa registrar na média\n"
        "  │ uma estrutura que ninguém preencheu — lixo, com cara de\n"
        "  │ medida. Sem quadro completo, a resposta é false, e isso não\n"
        "  │ é erro: é o caso comum.\n");
}


void test_um_quadro_completo_e_lido(void) {
    pm_init();

    const bool injetou = falso_pms_injetar_quadro(30, 30, 30, 8, 12, 19);
    VERIFICA(injetou, AVISO_UART_FECHADA);

    LeituraParticulas pm = pm_ler();
    VERIFICA(pm.ok == true,
        "  │ Um quadro válido de 32 bytes chegou pela UART e pm_ler()\n"
        "  │ voltou com .ok = false.\n");
}


// ═════════════════════════════════════════════════════════════════════════
//  TODO 7 — a família certa de valores
// ═════════════════════════════════════════════════════════════════════════

void test_usa_a_familia_do_ar_ambiente(void) {
    pm_init();

    // O quadro carrega as DUAS famílias ao mesmo tempo. Aqui elas foram
    // postas bem distantes de propósito: no sensor real, em ar limpo, as
    // duas dão quase o mesmo número, e pegar a errada é invisível.
    falso_pms_injetar_quadro(/* SP */ 90, 95, 99,
                             /* AE */  8, 12, 19);

    LeituraParticulas pm = pm_ler();

    VERIFICA(pm.pm1_0 == 8 && pm.pm2_5 == 12 && pm.pm10 == 19,
        "  │ Os valores lidos vieram da família errada, ou os três campos\n"
        "  │ estão trocados de lugar.\n"
        "  │\n"
        "  │ O quadro traz duas famílias com nomes parecidos:\n"
        "  │     PM_SP_*  'standard particles' — calibrada para laboratório\n"
        "  │     PM_AE_*  'atmospheric environment' — para ar ambiente\n"
        "  │\n"
        "  │ O dispositivo mede o ar de uma sala, então a certa é a AE:\n"
        "  │     r.pm1_0 = dados.PM_AE_UG_1_0;\n"
        "  │     r.pm2_5 = dados.PM_AE_UG_2_5;\n"
        "  │     r.pm10  = dados.PM_AE_UG_10_0;\n"
        "  │\n"
        "  │ O compilador aceita as duas sem reclamar, e na bancada, em ar\n"
        "  │ limpo, os números quase coincidem. Este teste existe porque\n"
        "  │ este é um erro que o hardware ESCONDE.\n");
}


void test_esvazia_o_buffer_e_fica_com_o_mais_recente(void) {
    pm_init();

    // Três quadros chegaram entre uma passagem do laço e a seguinte.
    falso_pms_injetar_quadro(0, 0, 0,  1,  2,  3);
    falso_pms_injetar_quadro(0, 0, 0, 10, 20, 30);
    falso_pms_injetar_quadro(0, 0, 0, 40, 50, 60);

    LeituraParticulas pm = pm_ler();

    VERIFICA(pm.pm2_5 == 50,
        "  │ Depois de três quadros acumulados, pm_ler() devolveu um valor\n"
        "  │ que não é o do quadro mais recente.\n"
        "  │\n"
        "  │ O laço  while (Serial2.available())  existe para esvaziar o\n"
        "  │ buffer e ficar com o último. Sem ele, cada chamada consome um\n"
        "  │ quadro antigo e as leituras vão atrasando em relação ao ar\n"
        "  │ real — um atraso que só cresce e que nada no número denuncia.\n");
}



// ═════════════════════════════════════════════════════════════════════════
//  TODO 8 — a ventoinha aquecendo
//
//  Os dois primeiros formam par, pelo mesmo motivo dos testes do ENS160:
//  sozinho, o primeiro passaria com um  r.ok = false  fixo. É preciso
//  provar as duas coisas — que a leitura NÃO vale cedo, e que ela VOLTA
//  a valer depois.
// ═════════════════════════════════════════════════════════════════════════

static const uint32_t AQUECIMENTO_DO_ENSAIO_MS = 30000;

void test_leitura_nao_vale_enquanto_a_ventoinha_aquece(void) {
    pm_definir_aquecimento_ms(AQUECIMENTO_DO_ENSAIO_MS);
    pm_init();

    host_avancar_ms(AQUECIMENTO_DO_ENSAIO_MS - 1000);   // falta 1 s
    falso_pms_injetar_quadro(0, 0, 0, 8, 12, 19);

    LeituraParticulas pm = pm_ler();

    VERIFICA(pm.ok == false,
        "  │ O quadro chegou inteiro e pm_ler() certificou a leitura, mas a\n"
        "  │ ventoinha ainda não girou o bastante desde que foi ligada.\n"
        "  │\n"
        "  │ Falta o TODO 8, dentro de pm_ler():\n"
        "  │     if (millis() - s_ventoinha_ligou_ms < s_aquecimento_ms) {\n"
        "  │         r.ok = false;\n"
        "  │         return r;\n"
        "  │     }\n"
        "  │\n"
        "  │ O ar que o sensor mede logo depois de a ventoinha ligar é o que\n"
        "  │ estava parado dentro da câmara, não o da sala. Diferente do\n"
        "  │ ENS160, o PMS5003 não avisa: entrega o número com toda a\n"
        "  │ confiança do mundo, e quem tem de lembrar é o firmware.\n");
}


void test_leitura_volta_a_valer_depois_do_aquecimento(void) {
    pm_definir_aquecimento_ms(AQUECIMENTO_DO_ENSAIO_MS);
    pm_init();

    host_avancar_ms(AQUECIMENTO_DO_ENSAIO_MS + 1000);   // passou
    falso_pms_injetar_quadro(0, 0, 0, 8, 12, 19);

    LeituraParticulas pm = pm_ler();

    // Só o .ok interessa aqui. Se o valor está certo é assunto do
    // test_usa_a_familia_do_ar_ambiente — misturar as duas cobranças faria
    // este teste falhar por um motivo e acusar outro.
    VERIFICA(pm.ok == true,
        "  │ Passado o aquecimento, a leitura tem de voltar a valer — e este\n"
        "  │ teste diz que ela não voltou.\n"
        "  │\n"
        "  │ O par com o teste anterior é proposital: uma função que devolve\n"
        "  │ sempre  .ok = false  passaria naquele e reprova neste. A\n"
        "  │ comparação do TODO 8 tem de ser com  <  (menor que), e o\n"
        "  │ return só acontece DENTRO do if.\n");
}


void test_acordar_reinicia_a_contagem_do_aquecimento(void) {
    pm_definir_aquecimento_ms(AQUECIMENTO_DO_ENSAIO_MS);
    pm_init();
    host_avancar_ms(AQUECIMENTO_DO_ENSAIO_MS + 1000);   // já esquentou

    // O ciclo de energia da Etapa 3 faz exatamente isto entre uma medição
    // e outra. Ao religar, o aquecimento recomeça do zero: a câmara voltou
    // a encher de ar parado enquanto a ventoinha esteve desligada.
    pm_dormir();
    pm_acordar();

    host_avancar_ms(1000);
    falso_pms_injetar_quadro(0, 0, 0, 8, 12, 19);

    LeituraParticulas pm = pm_ler();

    VERIFICA(pm.ok == false,
        "  │ Depois de pm_dormir() e pm_acordar(), a contagem do aquecimento\n"
        "  │ tem de RECOMEÇAR — e aqui ela não recomeçou.\n"
        "  │\n"
        "  │ Enquanto a ventoinha esteve parada, a câmara encheu de ar\n"
        "  │ parado outra vez; o sensor precisa dos mesmos segundos de\n"
        "  │ novo. Quem garante isso é a linha\n"
        "  │     s_ventoinha_ligou_ms = millis();\n"
        "  │ dentro de pm_acordar().\n");
}

// ═════════════════════════════════════════════════════════════════════════
int main(int argc, char** argv) {
    (void)argc; (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_init_abre_a_uart);
    RUN_TEST(test_init_usa_os_pinos_do_config);
    RUN_TEST(test_init_acorda_a_ventoinha);
    RUN_TEST(test_init_poe_em_modo_ativo);

    RUN_TEST(test_sem_quadro_nao_ha_leitura);
    RUN_TEST(test_um_quadro_completo_e_lido);
    RUN_TEST(test_usa_a_familia_do_ar_ambiente);
    RUN_TEST(test_esvazia_o_buffer_e_fica_com_o_mais_recente);

    RUN_TEST(test_leitura_nao_vale_enquanto_a_ventoinha_aquece);
    RUN_TEST(test_leitura_volta_a_valer_depois_do_aquecimento);
    RUN_TEST(test_acordar_reinicia_a_contagem_do_aquecimento);

    // Devolver 0 sempre: o PlatformIO interpreta um codigo de saida diferente
    // de zero como "o programa morreu", e o numero de falhas viraria o nome
    // de um sinal ("SIGABRT"). Quem decide se a suite passou e o proprio
    // PlatformIO, lendo as linhas de PASSED/FAILED acima.
    UNITY_END();
    return 0;
}
