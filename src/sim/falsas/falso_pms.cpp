#ifdef SIM_HOST

#include "falsas.h"
#include <Arduino.h>

// ═════════════════════════════════════════════════════════════════════════
//  O quadro do PMS5003, montado byte a byte.
//
//  Repare no que NÃO tem aqui: uma biblioteca PMS falsa. A biblioteca real
//  é usada como está — ela só precisa de um Stream, e o Stream falso
//  entrega bytes iguais aos que o sensor entregaria.
//
//  Isso importa mais do que parece. Se o quadro fosse "interpretado" por
//  um fake nosso, o teste estaria conferindo o nosso entendimento do
//  protocolo em vez do código que roda na placa. Assim, o parser sob teste
//  é exatamente o que vai rodar no ESP32.
//
//  O quadro (cap. 18 — o quadro da UART, num nível acima):
//
//    0x42 0x4D | tamanho (2 B) | 13 medidas de 2 B | checksum (2 B)
//    └ cabeçalho              └ 0x001C = 28 = 13*2 + 2
//
//  O checksum é a soma simples de todos os 30 bytes anteriores.
// ═════════════════════════════════════════════════════════════════════════

bool falso_pms_injetar_quadro(uint16_t sp1_0, uint16_t sp2_5, uint16_t sp10,
                              uint16_t ae1_0, uint16_t ae2_5, uint16_t ae10) {
    // O sensor de verdade não fala se estiver dormindo, e não fala sozinho
    // fora do modo ativo. Sem o wakeUp() e o activeMode() do TODO 6, o
    // silêncio aqui é o mesmo silêncio da bancada.
    if (!Serial2.foi_iniciada() || !Serial2.esta_acordado() ||
        !Serial2.em_modo_ativo()) {
        return false;
    }

    uint8_t q[32] = {0};
    q[0] = 0x42;
    q[1] = 0x4D;
    q[2] = 0x00;
    q[3] = 0x1C;                      // 28 = 13 medidas de 2 bytes + 2

    const uint16_t medidas[13] = {
        sp1_0, sp2_5, sp10,           // Standard Particles (laboratório)
        ae1_0, ae2_5, ae10,           // Atmospheric Environment (ar ambiente)
        320, 90, 12, 3, 1, 0, 0       // contagens por faixa de tamanho
    };

    for (int i = 0; i < 13; i++) {
        q[4 + i * 2]     = (uint8_t)(medidas[i] >> 8);
        q[4 + i * 2 + 1] = (uint8_t)(medidas[i] & 0xFF);
    }

    uint16_t soma = 0;
    for (int i = 0; i < 30; i++) {
        soma += q[i];
    }
    q[30] = (uint8_t)(soma >> 8);
    q[31] = (uint8_t)(soma & 0xFF);

    Serial2.injetar(q, sizeof(q));
    return true;
}

#endif  // SIM_HOST
