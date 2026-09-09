#include "sensor_amb.h"
#include "config.h"

#include <Wire.h>
#include <DFRobot_ENS160.h>
#include <Adafruit_AHTX0.h>

// 'static' aqui significa: estes objetos pertencem a este arquivo e
// ninguém de fora consegue mexer neles. É assim que se esconde o
// "como funciona" atrás do contrato declarado no .h.
static DFRobot_ENS160_I2C s_ens160(&Wire, ENS160_ENDERECO);
static Adafruit_AHTX0     s_aht;

// Os quatro estados que o ENS160 informa sobre si mesmo.
#define ENS160_OPERACAO_NORMAL   0
#define ENS160_AQUECENDO         1
#define ENS160_PARTIDA_INICIAL   2
#define ENS160_SAIDA_INVALIDA    3

static uint8_t s_status = ENS160_SAIDA_INVALIDA;


// ═════════════════════════════════════════════════════════════════════════
//  Preparar os dois sensores
// ═════════════════════════════════════════════════════════════════════════

bool amb_init() {
    Wire.begin();   // pinos padrão do ESP32: SDA = GPIO21, SCL = GPIO22

    // ─────────────────────────────────────────────────────────────────────
    // TODO 3 — Inicializar os DOIS sensores e verificar CADA UM.
    //
    // Cuidado: as duas bibliotecas usam convenções OPOSTAS de erro.
    //
    //   a) AHT21 — devolve bool, true = respondeu:
    //          if (!s_aht.begin()) {
    //              Serial.println("[Amb] AHT21 nao respondeu (esperado em 0x38).");
    //              return false;
    //          }
    //
    //   b) ENS160 — devolve int, e o sucesso é ZERO (a constante NO_ERR):
    //          if (s_ens160.begin() != NO_ERR) {
    //              Serial.printf("[Amb] ENS160 nao respondeu em 0x%02X.\n",
    //                            ENS160_ENDERECO);
    //              Serial.println("      Rode o scanner: muitos modulos usam 0x52.");
    //              return false;
    //          }
    //
    //   c) coloque o ENS160 em modo de medição contínua:
    //          s_ens160.setPWRMode(ENS160_STANDARD_MODE);
    //
    // Duas bibliotecas, duas convenções contrárias, no mesmo barramento.
    // Escrever if (s_ens160.begin()) "funciona" e está ERRADO: zero é
    // sucesso, então o if dispara justamente quando deu certo. Este é o
    // tipo de erro que só se evita lendo a documentação em vez de presumir.
    // ─────────────────────────────────────────────────────────────────────
    // AHT21 — devolve bool, e true significa "respondeu"
    if (!s_aht.begin()) {
    Serial.println("[Amb] AHT21 nao respondeu (esperado em 0x38).");
    return false;
    }
    // ENS160 — devolve int, e o sucesso é ZERO
    if (s_ens160.begin() != NO_ERR) {
    Serial.printf("[Amb] ENS160 nao respondeu em 0x%02X.\n", ENS160_ENDERECO);
    return false;
    }
    s_ens160.setPWRMode(ENS160_STANDARD_MODE);

    Serial.println("[Amb] O ENS160 leva alguns minutos aquecendo antes de");
    Serial.println("      os valores de gas valerem. Isso e normal.");
    return true;
}


// ═════════════════════════════════════════════════════════════════════════
//  Uma leitura completa
// ═════════════════════════════════════════════════════════════════════════

LeituraAmbiente amb_ler() {
    // Uma estrutura nova a cada leitura, com TODOS os campos zerados.
    // O  = {}  faz isso: sem ele, os campos começariam com lixo de memória,
    // e um campo que você esquecesse de preencher traria um número
    // qualquer com cara de medida.
    LeituraAmbiente r = {};

    // A biblioteca da Adafruit entrega as duas medidas em "eventos" — um
    // formato padronizado que ela usa para todos os sensores dela. Os  &
    // aqui são exigência DELA: ela precisa dos endereços das duas variáveis
    // para escrever dentro delas. Esta linha já vem pronta.
    sensors_event_t evt_umidade, evt_temperatura;

    if (!s_aht.getEvent(&evt_umidade, &evt_temperatura)) {
        Serial.println("[Amb] Falha ao ler o AHT21.");
        return r;              // r.ok continua false: nada aqui vale
    }

    r.temperatura = evt_temperatura.temperature;
    r.umidade     = evt_umidade.relative_humidity;
    s_ens160.setTempAndHum(r.temperatura, r.umidade); // Compensação do ENS160

    // ─────────────────────────────────────────────────────────────────────
    // TODO 4 — Compensação: contar ao ENS160 em que ar ele está medindo.
    //
    // O ENS160 é um sensor de óxido metálico: a resistência do elemento
    // dele muda com a temperatura e com a umidade, não só com os gases.
    // Sem informar as duas, o eCO2 varia com o clima do dia em vez de
    // variar com o ar da sala.
    //
    // É por isso que os dois sensores estão no MESMO módulo: um depende
    // do outro. Separá-los em dois arquivos obrigaria um a conhecer o
    // outro por fora, que é pior.
    // ─────────────────────────────────────────────────────────────────────

    // ─────────────────────────────────────────────────────────────────────
    // TODO 5 — Ler os gases e, principalmente, o ESTADO do sensor.
    //
    //   s_status = s_ens160.getENS160Status();
    //
    //   r.aqi  = s_ens160.getAQI();     // 1 a 5
    //   r.tvoc = s_ens160.getTVOC();    // ppb
    //   r.eco2 = s_ens160.getECO2();    // ppm
    //
    //   r.gases_validos = (s_status == ENS160_OPERACAO_NORMAL);
    //
    // Repare no que este último campo faz: o sensor está DIZENDO que ainda
    // não confia em si mesmo, e nós anotamos isso em vez de ignorar. Quem
    // chamou decide o que fazer — e no main.cpp a decisão será não deixar
    // esses valores entrarem na média.
    //
    // É a mesma ideia da leitura que falha, num disfarce mais educado:
    // um número existir não significa que ele valha.
    // ─────────────────────────────────────────────────────────────────────
    s_status = s_ens160.getENS160Status();

    r.aqi = s_ens160.getAQI();      // 1 a 5
    r.tvoc = s_ens160.getTVOC();    // ppb
    r.eco2 = s_ens160.getECO2();    // ppm

    r.gases_validos = (s_status == ENS160_OPERACAO_NORMAL);;   // 

    r.ok = true;               // chegou até aqui: a leitura vale
    return r;
}


// ═════════════════════════════════════════════════════════════════════════
//  Textos para o log — prontos, nada a fazer aqui
// ═════════════════════════════════════════════════════════════════════════

const char* amb_texto_status() {
    switch (s_status) {
        case ENS160_OPERACAO_NORMAL: return "operacao normal";
        case ENS160_AQUECENDO:       return "aquecendo";
        case ENS160_PARTIDA_INICIAL: return "partida inicial";
        default:                     return "saida invalida";
    }
}

const char* amb_texto_aqi(uint8_t aqi) {
    switch (aqi) {
        case 1:  return "excelente";
        case 2:  return "bom";
        case 3:  return "moderado";
        case 4:  return "ruim";
        case 5:  return "insalubre";
        default: return "indefinido";
    }
}
