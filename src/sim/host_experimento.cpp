#ifdef SIM_EXPERIMENTO

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "host/host.h"
#include "falsas/falsas.h"
#include "../config.h"

// ═════════════════════════════════════════════════════════════════════════
//  A BANCADA DE MEDIÇÃO DA MÉDIA MÓVEL
//
//      pio run -e experimento -t exec
//
//  O modo  -e native  mostra o firmware rodando em tempo real, uma linha
//  por segundo. É bom para ver o programa se comportar, e é péssimo para
//  MEDIR: ninguém compara "quanto tremeu" de um log que rolou faz três
//  minutos com o de agora.
//
//  Este modo faz a mesma coisa que um engenheiro faria na bancada com um
//  gerador de sinais: submete o filtro a um ensaio controlado e anota
//  números. São dois ensaios, na mesma linha do tempo:
//
//    1. AMBIENTE PARADO — o ar não muda, só há ruído. Mede-se o desvio
//       padrão da coluna crua e o da coluna med. A razão entre os dois é
//       o quanto a média acalmou a leitura.
//
//    2. DEGRAU — o ar muda de uma vez. Mede-se quantos segundos a coluna
//       med leva para cobrir 90% do salto. É o preço do item 1.
//
//  O tempo aqui é simulado: três minutos de ensaio passam num piscar.
//  E a semente do ruído é fixa, então a sequência de ruído é IDÊNTICA
//  em toda execução — trocar o TAMANHO_HISTORICO e rodar de novo é uma
//  comparação justa, com o mesmo ruído nos dois casos.
// ═════════════════════════════════════════════════════════════════════════

void setup();
void loop();

// ── O mesmo ruído do modo de tempo real, para os dois modos concordarem ──
static float ruido(float amplitude) {
    return amplitude * (2.0f * (rand() / (float)RAND_MAX) - 1.0f);
}

// ── Os patamares do ensaio ───────────────────────────────────────────────
static const float T_ANTES  = 24.0f,  T_DEPOIS  = 28.0f;   // °C
static const float PM_ANTES = 12.0f,  PM_DEPOIS = 30.0f;   // µg/m³

// Quanto tempo esperar antes de começar a medir. O histórico precisa estar
// cheio, senão o desvio medido seria o do transitório de enchimento e não
// o do regime. Depende do N escolhido, por isso não é um número fixo.
static const uint32_t ASSENTAR_S = (uint32_t)TAMANHO_HISTORICO + 30;
static const uint32_t MEDIR_S    = 600;   // janela do ensaio 1
static const uint32_t APOS_S     = (uint32_t)TAMANHO_HISTORICO * 3 + 40;


// ═════════════════════════════════════════════════════════════════════════
//  Leitura do log
//
//  Assim como o test_ciclo, esta bancada não enxerga as seis médias: elas
//  são static dentro do main.cpp. O que ela lê é o que o firmware
//  IMPRIMIU — a única saída observável do programa.
// ═════════════════════════════════════════════════════════════════════════

static bool ler_temperatura(const std::string& l, float* cru, float* med) {
    return sscanf(l.c_str(), "T %f C (med %f)", cru, med) == 2;
}

static bool ler_pm2_5(const std::string& l, float* cru, float* med) {
    unsigned pm1_0, pm2_5;
    if (sscanf(l.c_str(), "PM1.0 %u | PM2.5 %u (med %f)",
               &pm1_0, &pm2_5, med) != 3) {
        return false;
    }
    *cru = (float)pm2_5;
    return true;
}

typedef bool (*Leitor)(const std::string&, float*, float*);

// Colhe as amostras de uma faixa do log. `ate` = 0 significa "até o fim".
static void colher(Leitor leitor, size_t de, size_t ate,
                   std::vector<float>* cru, std::vector<float>* med) {
    const std::vector<std::string>& log = host_log();
    if (ate == 0 || ate > log.size()) {
        ate = log.size();
    }
    for (size_t i = de; i < ate; i++) {
        float c, m;
        if (leitor(log[i], &c, &m)) {
            cru->push_back(c);
            med->push_back(m);
        }
    }
}

static float desvio_padrao(const std::vector<float>& v) {
    if (v.size() < 2) return 0.0f;

    float soma = 0.0f;
    for (float x : v) soma += x;
    const float media = soma / v.size();

    float acumulado = 0.0f;
    for (float x : v) acumulado += (x - media) * (x - media);
    return sqrtf(acumulado / (v.size() - 1));
}

// Quantos segundos até a série cobrir 90% do salto de `antes` para `depois`.
// Devolve -1 se ela não chegou lá dentro da janela observada.
static int segundos_ate_90(const std::vector<float>& v,
                           float antes, float depois) {
    const float alvo = antes + 0.9f * (depois - antes);
    for (size_t i = 0; i < v.size(); i++) {
        if ((depois > antes && v[i] >= alvo) ||
            (depois < antes && v[i] <= alvo)) {
            return (int)i + 1;          // uma amostra por segundo
        }
    }
    return -1;
}


// ═════════════════════════════════════════════════════════════════════════
//  O ensaio
// ═════════════════════════════════════════════════════════════════════════

// Mantém o ambiente falso nos valores pedidos e roda o firmware por
// `segundos` de tempo simulado, do mesmo jeito que o modo de tempo real
// faria — mas sem esperar.
static void rodar(uint32_t segundos, float temperatura, float pm2_5) {
    static unsigned long ultimo_quadro = 0;

    const unsigned long fim = millis() + segundos * 1000UL;

    while (millis() < fim) {
        falso_aht_definir(temperatura + ruido(0.35f), 55.0f + ruido(1.2f));
        falso_ens_definir((uint16_t)(480 + ruido(25.0f)),
                          (uint16_t)(90 + ruido(12.0f)), 2);

        if (millis() - ultimo_quadro >= 1000) {
            ultimo_quadro = millis();
            float v = pm2_5 + ruido(3.5f);
            if (v < 0) v = 0;
            const uint16_t ae = (uint16_t)v;
            falso_pms_injetar_quadro(ae * 3, ae * 3, ae * 3,
                                     (uint16_t)(ae * 0.65f), ae,
                                     (uint16_t)(ae * 1.45f + 2));
        }

        loop();
        delay(5);       // com o relógio simulado, isto só adianta o tempo
    }
}

static void linha_ruido(const char* nome, const std::vector<float>& cru,
                        const std::vector<float>& med) {
    const float dc = desvio_padrao(cru);
    const float dm = desvio_padrao(med);

    printf("  %-14s %10.3f %12.3f", nome, dc, dm);
    if (dm > 0.0001f) {
        printf("  %19.1fx\n", dc / dm);
    } else {
        printf("  %20s\n", "--");
    }
}


int main() {
    host_ecoar(false);      // aqui interessa o relatório, não o log corrido
    srand(1);               // ruído sempre igual: a comparação entre N é justa

    printf("\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("  CARACTERIZACAO DA MEDIA MOVEL  —  TAMANHO_HISTORICO = %d\n",
           TAMANHO_HISTORICO);
    printf("  (mude o valor em src/config.h e rode de novo)\n");
    printf("════════════════════════════════════════════════════════════════\n\n");

    setup();

    // ── Ensaio 1: ambiente parado ────────────────────────────────────────
    rodar(ASSENTAR_S, T_ANTES, PM_ANTES);       // enche o histórico
    const size_t marco_medida = host_log().size();

    rodar(MEDIR_S, T_ANTES, PM_ANTES);          // a janela que vale
    const size_t marco_degrau = host_log().size();

    // ── Ensaio 2: o degrau ───────────────────────────────────────────────
    rodar(APOS_S, T_DEPOIS, PM_DEPOIS);

    // ── Apuração ─────────────────────────────────────────────────────────
    std::vector<float> t_cru, t_med, p_cru, p_med;
    colher(ler_temperatura, marco_medida, marco_degrau, &t_cru, &t_med);
    colher(ler_pm2_5,       marco_medida, marco_degrau, &p_cru, &p_med);

    std::vector<float> t_cru_d, t_med_d, p_cru_d, p_med_d;
    colher(ler_temperatura, marco_degrau, 0, &t_cru_d, &t_med_d);
    colher(ler_pm2_5,       marco_degrau, 0, &p_cru_d, &p_med_d);

    // ── Antes de relatar: o firmware está mesmo pronto? ──────────────────
    if (t_cru.size() < 10) {
        printf("Nenhuma linha de temperatura apareceu no log.\n");
        printf("Termine o TODO 3 (amb_init) e o TODO 9 (main.cpp) antes\n");
        printf("de rodar o experimento.\n\n");
        return 0;
    }
    if (p_cru.size() < 10) {
        printf("Linhas de temperatura apareceram, mas nenhuma de PM2.5.\n");
        printf("Termine os TODO 6 e 7 (sensor_pm.cpp) antes de rodar\n");
        printf("o experimento.\n\n");
        return 0;
    }
    if (desvio_padrao(t_med) < 0.0001f && desvio_padrao(p_med) < 0.0001f) {
        printf("As colunas  med  nao se mexem: a media movel ainda devolve\n");
        printf("sempre o mesmo valor. Termine os TODO 1 e 2 (media.cpp).\n\n");
        return 0;
    }

    // ── Ensaio 1 ─────────────────────────────────────────────────────────
    printf("ENSAIO 1 — ambiente PARADO por %u s: quanto cada coluna treme\n",
           MEDIR_S);
    printf("(desvio padrao; quanto menor, mais quieta a leitura)\n\n");
    printf("  %-14s %10s %12s %21s\n",
           "grandeza", "col. crua", "col. med", "ficou mais quieta");
    printf("  ---------------------------------------------------------------\n");
    linha_ruido("temperatura", t_cru, t_med);
    linha_ruido("PM2.5",       p_cru, p_med);

    // ── Ensaio 2 ─────────────────────────────────────────────────────────
    const int t_cru_s = segundos_ate_90(t_cru_d, T_ANTES,  T_DEPOIS);
    const int t_med_s = segundos_ate_90(t_med_d, T_ANTES,  T_DEPOIS);
    const int p_cru_s = segundos_ate_90(p_cru_d, PM_ANTES, PM_DEPOIS);
    const int p_med_s = segundos_ate_90(p_med_d, PM_ANTES, PM_DEPOIS);

    printf("\n");
    printf("ENSAIO 2 — DEGRAU no ambiente: quanto a media demora a acompanhar\n");
    printf("(segundos ate a coluna cobrir 90%% do salto)\n\n");
    printf("  %-14s %-18s %12s %12s\n",
           "grandeza", "degrau", "col. crua", "col. med");
    printf("  ---------------------------------------------------------------\n");
    printf("  %-14s %-18s %10d s %10d s\n",
           "temperatura", "24 -> 28 C",     t_cru_s, t_med_s);
    printf("  %-14s %-18s %10d s %10d s\n",
           "PM2.5",       "12 -> 30 ug/m3", p_cru_s, p_med_s);

    printf("\n");
    printf("Anote os dois numeros na tabela do roteiro e repita com outro N.\n");
    printf("\n");
    return 0;
}

#endif  // SIM_EXPERIMENTO
