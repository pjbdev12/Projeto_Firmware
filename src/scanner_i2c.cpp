// ═════════════════════════════════════════════════════════════════════════
// Ferramenta auxiliar — Scanner do barramento I2C
//
// Este arquivo NÃO faz parte do firmware. Ele existe para responder uma
// única pergunta, antes de qualquer programação:
//
//        "o ESP32 está enxergando os sensores?"
//
// Como usar:
//     pio run -e scanner -t upload -t monitor
//
// Para voltar ao firmware normal:
//     pio run -t upload -t monitor
//
// Neste projeto o scanner deve encontrar DOIS dispositivos no módulo
// combinado: o AHT21 e o ENS160. Se aparecer só um, o problema é de
// ligação ou de endereço — e nenhuma linha de código resolve isso.
//
// (O PMS5003 não aparece aqui: ele é UART, não I2C. São barramentos
//  diferentes, e o scanner só enxerga um deles.)
// ═════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <Wire.h>

// O que esperamos encontrar neste projeto.
static const char* quem_e(uint8_t endereco) {
    switch (endereco) {
        case 0x38: return "AHT21  (temperatura e umidade)";
        case 0x39: return "AHT21 em endereco alternativo";
        case 0x52: return "ENS160 (gases) — ajuste ENS160_ENDERECO para 0x52";
        case 0x53: return "ENS160 (gases)";
        default:   return "dispositivo nao esperado neste projeto";
    }
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Wire.begin();               // SDA = GPIO21, SCL = GPIO22
    Serial.println("\n=== Scanner I2C ===");
}

void loop() {
    int encontrados = 0;

    Serial.println("Varrendo enderecos 0x01 a 0x7E...");

    for (uint8_t endereco = 1; endereco < 127; endereco++) {
        // Tenta iniciar uma conversa com este endereço. Se alguém responder,
        // endTransmission() devolve 0.
        Wire.beginTransmission(endereco);

        if (Wire.endTransmission() == 0) {
            Serial.printf("  -> 0x%02X  %s\n", endereco, quem_e(endereco));
            encontrados++;
        }
    }

    if (encontrados == 0) {
        Serial.println("  Nenhum dispositivo encontrado. Confira, nesta ordem:");
        Serial.println("   1. o modulo esta alimentado com 3,3 V (nao 5 V)?");
        Serial.println("   2. SDA e SCL estao trocados? (e o erro mais comum)");
        Serial.println("   3. os fios estao bem encaixados na protoboard?");
        Serial.println("   4. o GND do modulo esta ligado ao GND da placa?");
    } else {
        Serial.printf("  Total: %d dispositivo(s). O esperado neste projeto e 2.\n",
                      encontrados);
    }

    Serial.println();
    delay(3000);
}
