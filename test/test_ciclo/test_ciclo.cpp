// ═════════════════════════════════════════════════════════════════════════
//  Testes do ciclo completo — TODO 9.
//
//      pio test -e native -v
//
//  Os outros arquivos de teste chamam uma função e olham o que ela devolve.
//  Este é diferente: ele roda o SEU setup() e o SEU loop(), minuto após
//  minuto de tempo simulado, e depois lê o que o firmware imprimiu.
//
//  Por que pelo que foi impresso, e não pelas variáveis: as seis médias são
//  'static' dentro do main.cpp — de fora, ninguém as enxerga, e isso é o
//  correto. O log é a única saída observável do programa, no PC como na
//  bancada. Testar por ele é testar o que o dispositivo de fato comunica.
//
//  E o tempo aqui é de mentira: 40 segundos de firmware passam em alguns
//  milissegundos, porque quem move o relógio é o teste.
// ═════════════════════════════════════════════════════════════════════════

#include <unity.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "config.h"
#include "host.h"
#include "falsas.h"

// O firmware sob teste. Não incluímos main.cpp: declaramos o que ele
// oferece, que é exatamente o que o núcleo do Arduino chama na placa.
void setup();
void loop();

static void explica(const char* texto) {
    printf("\n"
           "  ┌─ o que este teste esperava ─────────────────────────────────\n"
           "%s"
           "  └──────────────────────────────────────────────────────────────\n\n",
           texto);
}

#define VERIFICA(condicao, explicacao)                                     \
    do {                                                                   \
        const bool resultado = (condicao);                                 \
        if (!resultado) { explica(explicacao); }                           \
        TEST_ASSERT_TRUE_MESSAGE(resultado, "(veja acima)");               \
    } while (0)


// ═════════════════════════════════════════════════════════════════════════
//  Ler de volta o que o firmware imprimiu
// ═════════════════════════════════════════════════════════════════════════

struct LinhaAmb {
    float    temperatura, med_temperatura;
    float    umidade,     med_umidade;
    unsigned eco2;        float med_eco2;
    unsigned tvoc;        float med_tvoc;
    unsigned aqi;
};

struct LinhaPM {
    unsigned pm1_0, pm2_5;  float med_pm2_5;
    unsigned pm10;          float med_pm10;
    int      n;
};

// O formato vem do printf() que o esqueleto já entrega pronto. Espaços no
// formato do sscanf casam com qualquer quantidade de espaços na linha.
static bool ler_amb(const std::string& linha, LinhaAmb& a) {
    return sscanf(linha.c_str(),
                  " T %f C (med %f) | UR %f %% (med %f) | eCO2 %u ppm (med %f)"
                  " | TVOC %u ppb (med %f) | AQI %u",
                  &a.temperatura, &a.med_temperatura,
                  &a.umidade, &a.med_umidade,
                  &a.eco2, &a.med_eco2,
                  &a.tvoc, &a.med_tvoc,
                  &a.aqi) == 9;
}

static bool ler_pm(const std::string& linha, LinhaPM& p) {
    return sscanf(linha.c_str(),
                  " PM1.0 %u | PM2.5 %u (med %f) | PM10 %u (med %f) ug/m3 (n = %d)",
                  &p.pm1_0, &p.pm2_5, &p.med_pm2_5,
                  &p.pm10, &p.med_pm10, &p.n) == 6;
}

static std::vector<LinhaAmb> linhas_amb() {
    std::vector<LinhaAmb> saida;
    LinhaAmb a;
    for (const auto& linha : host_log()) {
        if (ler_amb(linha, a)) saida.push_back(a);
    }
    return saida;
}

static std::vector<LinhaPM> linhas_pm() {
    std::vector<LinhaPM> saida;
    LinhaPM p;
    for (const auto& linha : host_log()) {
        if (ler_pm(linha, p)) saida.push_back(p);
    }
    return saida;
}


// ═════════════════════════════════════════════════════════════════════════
//  Rodar o firmware por um tempo
// ═════════════════════════════════════════════════════════════════════════

// Avança o relógio em passos curtos chamando loop() a cada um — como o
// núcleo do Arduino faz —, e entrega um quadro do PMS5003 por segundo,
// que é o ritmo do sensor de verdade.
static void rodar(uint32_t total_ms, uint16_t pm2_5 = 12) {
    const uint32_t passo = 100;
    uint32_t desde_o_quadro = 0;

    for (uint32_t t = 0; t < total_ms; t += passo) {
        if (desde_o_quadro >= 1000) {
            desde_o_quadro = 0;
            falso_pms_injetar_quadro(pm2_5 * 3, pm2_5 * 3, pm2_5 * 3,
                                     (uint16_t)(pm2_5 * 0.65f), pm2_5,
                                     (uint16_t)(pm2_5 * 1.45f + 2));
        }
        loop();
        host_avancar_ms(passo);
        desde_o_quadro += passo;
    }
}

void setUp(void) {
    host_reiniciar();
    falso_ens_aquecimento_ms(20000);
    falso_aht_definir(25.0f, 50.0f);
    falso_ens_definir(700, 120, 2);
    setup();
}

void tearDown(void) { }


// ═════════════════════════════════════════════════════════════════════════
//  TODO 9a — a temperatura e a umidade entram na média
// ═════════════════════════════════════════════════════════════════════════

void test_a_temperatura_entra_na_media(void) {
    rodar(10000);
    const auto linhas = linhas_amb();

    VERIFICA(linhas.size() >= 5,
        "  │ Em 10 segundos deveriam ter saído cerca de 10 linhas de\n"
        "  │ leitura, e saíram menos que 5. Ou o loop() não está lendo, ou\n"
        "  │ o formato do printf() foi alterado — ele é entregue pronto e\n"
        "  │ este teste depende dele.\n");

    const auto& ultima = linhas.back();
    VERIFICA(ultima.med_temperatura > 24.9f && ultima.med_temperatura < 25.1f,
        "  │ A temperatura ficou fixa em 25,0 °C o tempo todo, e a média\n"
        "  │ impressa não é 25,0.\n"
        "  │\n"
        "  │ Se saiu 0,0, falta  media_registrar(&s_temperatura, amb.temperatura)\n"
        "  │ no TODO 9a — ou a média móvel do TODO 1 e 2 ainda não calcula.\n");
}


void test_a_umidade_usa_a_sua_propria_media(void) {
    rodar(6000);
    const auto linhas = linhas_amb();
    const auto& ultima = linhas.back();

    VERIFICA(ultima.med_umidade > 49.9f && ultima.med_umidade < 50.1f,
        "  │ A umidade ficou fixa em 50,0 % e a média dela não é 50,0.\n"
        "  │\n"
        "  │ Se a média da umidade saiu igual à da temperatura, as duas\n"
        "  │ chamadas de media_registrar() estão recebendo a MESMA\n"
        "  │ MediaMovel. Confira os & do TODO 9a: cada grandeza tem a sua.\n");
}


void test_a_media_atrasa_em_relacao_a_leitura_crua(void) {
    falso_aht_definir(20.0f, 50.0f);
    rodar(10000);                       // enche o histórico com 20,0

    falso_aht_definir(30.0f, 50.0f);    // o ambiente muda de repente
    rodar(1500);                        // uma leitura nova, só

    const auto& ultima = linhas_amb().back();

    VERIFICA(ultima.temperatura > 29.9f,
        "  │ A leitura crua deveria ter ido para 30,0 imediatamente.\n");

    VERIFICA(ultima.med_temperatura > 20.0f && ultima.med_temperatura < 29.0f,
        "  │ A temperatura pulou de 20 para 30 de uma vez, e a média\n"
        "  │ acompanhou junto — ou não se mexeu.\n"
        "  │\n"
        "  │ É este o comportamento que a etapa inteira existe para\n"
        "  │ produzir: a coluna crua reage na hora, a média chega depois,\n"
        "  │ mais suave. Se as duas colunas são sempre iguais, o histórico\n"
        "  │ guarda uma leitura só.\n");
}


// ═════════════════════════════════════════════════════════════════════════
//  TODO 9b — o coração da etapa
// ═════════════════════════════════════════════════════════════════════════

void test_gas_do_aquecimento_nao_entra_na_media(void) {
    rodar(15000);                       // o aquecimento dura 20 s
    const auto linhas = linhas_amb();

    int contaminadas = 0;
    for (const auto& l : linhas) {
        if (l.med_eco2 != 0.0f) contaminadas++;
    }

    VERIFICA(contaminadas == 0,
        "  │ O ENS160 ainda estava aquecendo — ele próprio reportando que\n"
        "  │ não confia na saída — e a média de eCO2 já tinha valor.\n"
        "  │\n"
        "  │ Durante o aquecimento o sensor devolve 400 fixo. Registrar\n"
        "  │ esses valores enche a média de 400s, e o sintoma aparece muito\n"
        "  │ depois, disfarçado de 'esse sensor é impreciso'.\n"
        "  │\n"
        "  │ Falta o if do TODO 9b em volta das duas chamadas dos gases:\n"
        "  │     if (amb.gases_validos) {\n"
        "  │         media_registrar(&s_eco2, amb.eco2);\n"
        "  │         media_registrar(&s_tvoc, amb.tvoc);\n"
        "  │     }\n");
}


void test_gas_confiavel_entra_na_media(void) {
    rodar(30000);                       // passa do aquecimento
    const auto linhas = linhas_amb();

    bool alguma_com_media = false;
    for (const auto& l : linhas) {
        if (l.med_eco2 > 0.0f) alguma_com_media = true;
    }

    VERIFICA(alguma_com_media,
        "  │ O aquecimento terminou, o sensor passou a reportar 'operacao\n"
        "  │ normal', e a média de eCO2 continuou zerada o tempo todo.\n"
        "  │\n"
        "  │ A condição do TODO 9b provavelmente está invertida: ela está\n"
        "  │ descartando justamente o que vale.\n"
        "  │\n"
        "  │ (Se o teste anterior também falhou, comece por ele: os dois\n"
        "  │ olham o mesmo if, de lados opostos.)\n");
}


// ═════════════════════════════════════════════════════════════════════════
//  TODO 9c — as partículas
// ═════════════════════════════════════════════════════════════════════════

void test_as_particulas_entram_na_media(void) {
    rodar(8000, /* pm2_5 = */ 12);
    const auto linhas = linhas_pm();

    VERIFICA(!linhas.empty(),
        "  │ Nenhuma linha de partículas apareceu no log.\n"
        "  │\n"
        "  │ No teste os quadros chegam a cada segundo, então isto não é\n"
        "  │ falta de sensor. Ou o TODO 6 não abriu a UART (rode a suíte\n"
        "  │ test_sensor_pm), ou o bloco B do loop() não usa pm_ler().\n");

    const auto& ultima = linhas.back();
    VERIFICA(ultima.med_pm2_5 > 11.9f && ultima.med_pm2_5 < 12.1f,
        "  │ Todos os quadros trouxeram PM2.5 = 12, e a média impressa não\n"
        "  │ é 12,0.\n"
        "  │\n"
        "  │ Falta  media_registrar(&s_pm2_5, pm.pm2_5);  no TODO 9c.\n");

    VERIFICA(ultima.n > 1,
        "  │ O contador de amostras (n) não passou de 1: cada quadro está\n"
        "  │ substituindo o anterior em vez de se somar ao histórico.\n");
}


// ═════════════════════════════════════════════════════════════════════════
int main(int argc, char** argv) {
    (void)argc; (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_a_temperatura_entra_na_media);
    RUN_TEST(test_a_umidade_usa_a_sua_propria_media);
    RUN_TEST(test_a_media_atrasa_em_relacao_a_leitura_crua);

    RUN_TEST(test_gas_do_aquecimento_nao_entra_na_media);
    RUN_TEST(test_gas_confiavel_entra_na_media);

    RUN_TEST(test_as_particulas_entram_na_media);

    // Devolver 0 sempre: o PlatformIO interpreta um codigo de saida diferente
    // de zero como "o programa morreu", e o numero de falhas viraria o nome
    // de um sinal ("SIGABRT"). Quem decide se a suite passou e o proprio
    // PlatformIO, lendo as linhas de PASSED/FAILED acima.
    UNITY_END();
    return 0;
}
