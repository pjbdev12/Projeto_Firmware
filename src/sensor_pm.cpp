#include "sensor_pm.h"
#include "config.h"

#include <PMS.h>

// Serial2 é a terceira UART do ESP32. A primeira (Serial) está ocupada
// com o monitor serial — usá-la para o sensor misturaria os dois fluxos
// e você veria lixo binário no meio das suas mensagens.
static PMS s_pms(Serial2);

// Quando a ventoinha foi ligada pela última vez, e quanto tempo ela precisa
// girar até o ar dentro da câmara ser o ar de fora. Enquanto não for, o
// sensor responde — e responde sobre o ar errado.
static unsigned long s_ventoinha_ligou_ms = 0;
static uint32_t      s_aquecimento_ms     = PM_AQUECIMENTO_MS;


void pm_definir_aquecimento_ms(uint32_t ms) {
    s_aquecimento_ms = ms;
}


// ═════════════════════════════════════════════════════════════════════════
//  Abrir a UART
// ═════════════════════════════════════════════════════════════════════════

bool pm_init() {
    // ─────────────────────────────────────────────────────────────────────
    // TODO 6 — Abrir a porta e acordar o sensor. Três linhas:
    //
    //   a) Serial2.begin(PMS_BAUD, SERIAL_8N1, PMS_PINO_RX, PMS_PINO_TX);
    //
    //      SERIAL_8N1 = 8 bits de dados, sem paridade, 1 stop bit — o
    //      formato de quadro da apostila cap. 18. Os dois últimos
    //      argumentos dizem em quais pinos a UART vai sair, porque no
    //      ESP32 quase qualquer pino serve.
    //
    //   b) s_pms.activeMode();
    //
    //      Modo ATIVO: o sensor manda um quadro por conta própria, mais ou
    //      menos uma vez por segundo, sem ninguém pedir. A alternativa
    //      (passiveMode) exige pedir e ESPERAR a resposta — o que
    //      bloquearia o laço, justamente o que este projeto evita.
    //
    //   c) s_pms.wakeUp();     // liga a ventoinha
    // ─────────────────────────────────────────────────────────────────────
    Serial.println("[PM] (TODO 6 pendente: UART ainda nao aberta)");

    // A partir daqui conta o aquecimento da ventoinha (TODO 8).
    s_ventoinha_ligou_ms = millis();
    return true;
}


// ═════════════════════════════════════════════════════════════════════════
//  Escutar o que o sensor mandou
//
//  Este sensor não responde a perguntas: ele fala sozinho. O nosso papel
//  é escutar sem parar o programa para isso.
// ═════════════════════════════════════════════════════════════════════════

LeituraParticulas pm_ler() {
    LeituraParticulas r = {};   // todos os campos zerados, inclusive r.ok
    PMS::DATA dados;

    // Cada chamada a read() consome UM byte do buffer da UART e devolve
    // true apenas no byte que fecha um quadro de 32 (cap. 18: o quadro).
    // Por isso o laço: esvaziamos tudo o que chegou e ficamos com o
    // quadro mais recente.
    //
    // Esvaziar importa. Sem isso o buffer acumula quadros antigos, e as
    // leituras vão ficando cada vez mais atrasadas em relação ao ar real —
    // um atraso que só cresce, e que ninguém percebe olhando o número.
    //
    // E repare: o laço termina quando o buffer esvazia. Ele NÃO espera
    // por um quadro que talvez nunca venha.
    while (Serial2.available()) {
        if (s_pms.read(dados)) {
            r.ok = true;
        }
    }

    if (!r.ok) {
        return r;   // ainda não chegou quadro completo; sem drama
    }

    // ─────────────────────────────────────────────────────────────────────
    // TODO 8 — Recusar a leitura enquanto a ventoinha estiver aquecendo.
    //
    //   if (millis() - s_ventoinha_ligou_ms < s_aquecimento_ms) {
    //       r.ok = false;
    //       return r;
    //   }
    //
    // O quadro chegou inteiro, com checksum correto, e mesmo assim pode não
    // valer. Quando a ventoinha liga, o ar que ela mede primeiro é o que
    // estava parado dentro da câmara — não o da sala.
    //
    // É o mesmo problema do aquecimento do ENS160 (TODO 5), com uma
    // diferença que muda tudo: o ENS160 AVISA. Ele tem um campo de estado, e
    // basta perguntar. O PMS5003 não avisa nada — entrega o número com toda
    // a confiança do mundo, e cabe ao firmware lembrar.
    //
    // Nesta etapa s_aquecimento_ms vale ZERO (veja o config.h), porque a
    // ventoinha liga no pm_init() e nunca mais para: não há o que esperar, e
    // este if nunca dispara. Ele existe porque a partir da Etapa 3 o
    // dispositivo passa a DESLIGAR a ventoinha entre um ciclo e outro — o
    // laser e o motor do PMS5003 têm vida útil contada em horas de
    // operação — e aí o aquecimento passa a acontecer de verdade, a cada
    // ciclo. Quem resolve a consequência é este módulo, aqui e agora; quem
    // decide quando desligar é a Etapa 3.
    // ─────────────────────────────────────────────────────────────────────

    // ─────────────────────────────────────────────────────────────────────
    // TODO 7 — Copiar os três valores para a estrutura de saída.
    //
    //   r.pm1_0 = dados.PM_AE_UG_1_0;
    //   r.pm2_5 = dados.PM_AE_UG_2_5;
    //   r.pm10  = dados.PM_AE_UG_10_0;
    //
    // O sensor informa DUAS famílias de valores, e é fácil pegar a errada:
    //
    //   PM_SP_* → "standard particles": calibrada para ar de laboratório.
    //   PM_AE_* → "atmospheric environment": calibrada para ar ambiente.
    //
    // Como o dispositivo vai medir o ar de uma sala, a família certa é a
    // AE. Os dois conjuntos existem lado a lado no mesmo quadro, com nomes
    // parecidos, e o compilador aceita qualquer um dos dois sem reclamar.
    // ─────────────────────────────────────────────────────────────────────

    return r;
}


// ═════════════════════════════════════════════════════════════════════════
//  Controle da ventoinha — usado de verdade só na Etapa 3
// ═════════════════════════════════════════════════════════════════════════

void pm_dormir() {
    s_pms.sleep();
    Serial.println("[PM] Ventoinha desligada.");
}

void pm_acordar() {
    s_pms.wakeUp();
    s_ventoinha_ligou_ms = millis();
    Serial.println("[PM] Ventoinha ligada; aquecendo.");
}
