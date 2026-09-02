// ═════════════════════════════════════════════════════════════════════════
// Etapa 1 — Os sensores
//
// O que este programa faz quando estiver pronto:
//   uma vez por segundo, lê os três sensores e imprime, lado a lado, a
//   leitura crua e a média das últimas N — para as seis grandezas.
//
// Soprando de leve perto do módulo você vê a diferença entre as duas
// colunas: a crua reage na hora, a média chega logo depois, mais suave.
//
// A arquitetura já está montada. O que falta está marcado com TODO,
// aqui e nos arquivos media.cpp, sensor_amb.cpp e sensor_pm.cpp.
// ═════════════════════════════════════════════════════════════════════════

#include <Arduino.h>

#include "config.h"
#include "media.h"
#include "sensor_amb.h"
#include "sensor_pm.h"

// Uma média móvel por grandeza. O mesmo tipo e as mesmas quatro funções
// servem a todas: é exatamente para isso que o histórico virou um struct.
static MediaMovel s_temperatura;
static MediaMovel s_umidade;
static MediaMovel s_eco2;
static MediaMovel s_tvoc;
static MediaMovel s_pm2_5;
static MediaMovel s_pm10;

// Guarda o instante da última leitura. 'unsigned long' porque millis()
// devolve um número que cresce sem parar e não cabe em um int.
static unsigned long s_ultima_leitura_ms = 0;

// Fica em false se os sensores I2C não responderam no boot.
static bool s_amb_ok = false;

// ═════════════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(200);                  // dá tempo do monitor serial abrir
    Serial.println("\n=== EFB2006 — Etapa 1: tres sensores, seis grandezas ===");

    media_zerar(&s_temperatura);
    media_zerar(&s_umidade);
    media_zerar(&s_eco2);
    media_zerar(&s_tvoc);
    media_zerar(&s_pm2_5);
    media_zerar(&s_pm10);

    s_amb_ok = amb_init();
    if (!s_amb_ok) {
        Serial.println("[Erro] Sensores I2C nao responderam.");
        Serial.println("       Rode o scanner I2C antes de mexer no codigo:");
        Serial.println("       pio run -e scanner -t upload -t monitor");
    }

    pm_init();
}


// ═════════════════════════════════════════════════════════════════════════

void loop() {

    // Só age quando já passou o intervalo. Entre uma leitura e outra o
    // loop() continua girando livremente — nada de delay() longo aqui.
    if (millis() - s_ultima_leitura_ms < INTERVALO_LEITURA_MS) {
        return;
    }

    s_ultima_leitura_ms = millis();

    // ── Bloco A: os sensores I2C ─────────────────────────────────────────
    if (s_amb_ok) {
        // amb_ler() DEVOLVE a leitura pronta. O campo .ok diz se ela vale.
        LeituraAmbiente amb = amb_ler();

        if (amb.ok) {
            // ─────────────────────────────────────────────────────────────
            // TODO 9a — Registrar temperatura e umidade nas médias.
            //
            //   media_registrar(&s_temperatura, amb.temperatura);
            //   media_registrar(&s_umidade,     amb.umidade);
            //
            // Este  &  é o ÚNICO ponteiro que você escreve no projeto
            // inteiro, e ele tem um motivo visível: existem seis médias e
            // uma só função. O  &  responde "em QUAL delas registrar".
            //
            // Sem ele, media_registrar() receberia uma cópia da média, e
            // atualizaria a cópia — a de verdade ficaria parada para
            // sempre, sem nenhum aviso (cap. 9 e 10).
            // ─────────────────────────────────────────────────────────────

            // ─────────────────────────────────────────────────────────────
            // TODO 9b — Registrar os gases APENAS se o sensor confiar neles.
            //
            //   if (amb.gases_validos) {
            //       media_registrar(&s_eco2, amb.eco2);
            //       media_registrar(&s_tvoc, amb.tvoc);
            //   }
            //
            // Este  if  é o coração da etapa. Sem ele, todos os valores do
            // aquecimento — que o próprio sensor marcou como não confiáveis —
            // entram na média e a contaminam. O sintoma aparece muito
            // depois...
            // ─────────────────────────────────────────────────────────────

            Serial.printf("T %5.1f C (med %5.1f) | UR %5.1f %% (med %5.1f) | "
                          "eCO2 %4u ppm (med %6.0f) | TVOC %4u ppb (med %6.0f) | "
                          "AQI %u %-10s [%s]\n",
                          amb.temperatura, media_valor(&s_temperatura),
                          amb.umidade,     media_valor(&s_umidade),
                          amb.eco2,        media_valor(&s_eco2),
                          amb.tvoc,        media_valor(&s_tvoc),
                          amb.aqi,         amb_texto_aqi(amb.aqi),
                          amb_texto_status());
        } else {
            Serial.println("[Aviso] Leitura I2C falhou; amostra descartada.");
        }
    }

    // ── Bloco B: o sensor de partículas ──────────────────────────────────
    LeituraParticulas pm = pm_ler();

    if (pm.ok) {
        // ─────────────────────────────────────────────────────────────────
        // TODO 9c — Registrar as partículas nas médias.
        //
        //   media_registrar(&s_pm2_5, pm.pm2_5);
        //   media_registrar(&s_pm10,  pm.pm10);
        //
        // Repare que aqui não há teste de validade: o  if (pm.ok)  lá em
        // cima já resolveu. O quadro chegou inteiro ou não chegou — não
        // existe meio-termo.
        // ─────────────────────────────────────────────────────────────────

        Serial.printf("PM1.0 %3u | PM2.5 %3u (med %5.1f) | PM10 %3u (med %5.1f)  ug/m3  "
                      "(n = %d)\n",
                      pm.pm1_0,
                      pm.pm2_5, media_valor(&s_pm2_5),
                      pm.pm10,  media_valor(&s_pm10),
                      media_quantidade(&s_pm2_5));
    }
}
