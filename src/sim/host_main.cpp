#ifdef SIM_HOST
#ifndef UNIT_TEST
#ifndef SIM_EXPERIMENTO   // o modo de medicao tem o main() dele

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>

#include "host/host.h"
#include "falsas/falsas.h"

// ═════════════════════════════════════════════════════════════════════════
//  O FIRMWARE RODANDO NO SEU COMPUTADOR
//
//      pio run -e native -t exec
//
//  Sem placa, sem cabo, sem sensor: o mesmo setup() e o mesmo loop() que
//  vão para o ESP32, com o relógio andando em velocidade normal e os
//  sensores falsos no lugar dos de verdade.
//
//  Não substitui a placa — não há timer de hardware, nem light sleep, nem
//  rádio. Serve para ver o programa se comportar, que na Etapa 1 é quase
//  tudo o que ele faz.
//
//  (Nos testes este main não existe: o PlatformIO define UNIT_TEST e quem
//  manda no relógio passa a ser o teste.)
// ═════════════════════════════════════════════════════════════════════════

void setup();
void loop();

// Ruído branco simples, no intervalo [-amplitude, +amplitude]. Um sensor
// real nunca devolve o mesmo número duas vezes seguidas, mesmo num
// ambiente parado — e é exatamente isso que a média móvel existe para
// atenuar. Sem ruído aqui, o experimento da etapa não teria o que medir.
static float ruido(float amplitude) {
    return amplitude * (2.0f * (rand() / (float)RAND_MAX) - 1.0f);
}

int main() {
    host_ecoar(true);              // o que o firmware imprime vai para a tela
    host_usar_relogio_real(true);  // 1 segundo é 1 segundo
    srand(1);                      // ruído sempre igual: o experimento repete

    printf("┌──────────────────────────────────────────────────────────┐\n");
    printf("│  Firmware rodando no PC, com sensores falsos.            │\n");
    printf("│  O ENS160 falso passa 20 s aquecendo. Ctrl-C para sair.  │\n");
    printf("└──────────────────────────────────────────────────────────┘\n");

    setup();

    unsigned long ultimo_quadro = 0;

    while (true) {
        const float t = millis() / 1000.0f;

        // Um ambiente que se mexe devagar (um ciclo a cada ~88 s), para a
        // diferença entre a coluna crua e a coluna da média ficar visível.
        const float lenta = sinf(t / 14.0f);

        // E, por cima, RUÍDO — que é a razão de a média móvel existir.
        // Repare nas amplitudes: o material particulado é muito mais
        // ruidoso que a temperatura. É a pergunta que fecha a etapa: um
        // mesmo TAMANHO_HISTORICO serve bem para as duas grandezas?
        falso_aht_definir(24.0f + 1.5f * lenta + ruido(0.35f),
                          55.0f - 4.0f * lenta + ruido(1.2f));
        falso_ens_definir((uint16_t)(480 + 60 * lenta + ruido(25.0f)),
                          (uint16_t)(90 + 25 * lenta + ruido(12.0f)), 2);

        // O PMS5003 fala sozinho, cerca de uma vez por segundo. A família
        // SP sai propositalmente diferente da AE: se o TODO 7 pegar a
        // errada, os números do log ficam três vezes maiores.
        if (millis() - ultimo_quadro >= 1000) {
            ultimo_quadro = millis();
            float v = 12 + 4 * lenta + ruido(3.5f);
            if (v < 0) v = 0;
            const uint16_t ae = (uint16_t)v;
            falso_pms_injetar_quadro(ae * 3, ae * 3, ae * 3,
                                     (uint16_t)(ae * 0.65f), ae,
                                     (uint16_t)(ae * 1.45f + 2));
        }

        loop();
        delay(5);
    }
}

#endif  // SIM_EXPERIMENTO
#endif  // UNIT_TEST
#endif  // SIM_HOST
