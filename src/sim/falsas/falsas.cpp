#include "Adafruit_AHTX0.h"
#include "DFRobot_ENS160.h"
#include "falsas.h"

// ═════════════════════════════════════════════════════════════════════════
//  O comportamento dos sensores falsos.
//
//  Duas regras guiaram o que está aqui:
//
//  1. Imitar o sensor real onde ele é traiçoeiro — o aquecimento do
//     ENS160, o eCO2 preso em 400, a leitura que falha de vez em quando.
//
//  2. Tornar VISÍVEL o que o sensor real esconde. O melhor exemplo é a
//     compensação: esquecer setTempAndHum() num ENS160 de verdade só
//     desloca um pouco o eCO2, e ninguém nota nunca. Aqui o sensor falso
//     reclama em voz alta.
// ═════════════════════════════════════════════════════════════════════════

// Os estados que o ENS160 informa sobre si mesmo.
#define STATUS_OPERACAO_NORMAL   0
#define STATUS_AQUECENDO         1
#define STATUS_PARTIDA_INICIAL   2
#define STATUS_SAIDA_INVALIDA    3

namespace {

struct EstadoFalso {
    // AHT21
    float temperatura      = 24.0f;
    float umidade          = 55.0f;
    bool  aht_falha_init   = false;
    bool  aht_falha_leitura = false;
    bool  aht_iniciado     = false;

    // ENS160
    uint16_t      eco2            = 480;
    uint16_t      tvoc            = 90;
    uint8_t       aqi             = 2;
    bool          ens_falha_init  = false;
    bool          ens_iniciado    = false;
    uint8_t       ens_modo        = 0xFF;
    unsigned long ens_ligou_ms    = 0;
    uint32_t      ens_aquecimento = 20000;

    bool  compensado         = false;
    float temp_compensacao   = 0.0f;
    float umid_compensacao   = 0.0f;
};

EstadoFalso e;

}  // namespace


void falsas_reiniciar() {
    e = EstadoFalso();
}


// ═════════════════════════════════════════════════════════════════════════
//  AHT21
// ═════════════════════════════════════════════════════════════════════════

bool Adafruit_AHTX0::begin() {
    if (e.aht_falha_init) {
        return false;          // convenção desta biblioteca: false = falhou
    }
    e.aht_iniciado = true;
    return true;
}

bool Adafruit_AHTX0::getEvent(sensors_event_t* umidade,
                              sensors_event_t* temperatura) {
    if (e.aht_falha_leitura) {
        return false;
    }
    temperatura->temperature      = e.temperatura;
    temperatura->relative_humidity = 0.0f;
    umidade->relative_humidity    = e.umidade;
    umidade->temperature          = 0.0f;
    return true;
}


// ═════════════════════════════════════════════════════════════════════════
//  ENS160
// ═════════════════════════════════════════════════════════════════════════

int DFRobot_ENS160_I2C::begin() {
    if (e.ens_falha_init) {
        return ERR_DATA_BUS;   // convenção OPOSTA: zero é sucesso
    }
    e.ens_iniciado = true;
    e.ens_ligou_ms = millis();
    return NO_ERR;
}

void DFRobot_ENS160_I2C::setPWRMode(uint8_t modo) {
    e.ens_modo = modo;
}

void DFRobot_ENS160_I2C::setTempAndHum(float temperatura, float umidade) {
    e.compensado       = true;
    e.temp_compensacao = temperatura;
    e.umid_compensacao = umidade;
}

uint8_t DFRobot_ENS160_I2C::getENS160Status() {
    if (!e.ens_iniciado) {
        return STATUS_SAIDA_INVALIDA;
    }
    return (millis() - e.ens_ligou_ms < e.ens_aquecimento)
               ? STATUS_AQUECENDO
               : STATUS_OPERACAO_NORMAL;
}

uint8_t DFRobot_ENS160_I2C::getAQI() {
    return (getENS160Status() == STATUS_OPERACAO_NORMAL) ? e.aqi : 1;
}

uint16_t DFRobot_ENS160_I2C::getTVOC() {
    return (getENS160Status() == STATUS_OPERACAO_NORMAL) ? e.tvoc : 0;
}

uint16_t DFRobot_ENS160_I2C::getECO2() {
    // Aquecendo, o sensor de verdade devolve 400 e não se mexe. É o
    // sintoma que o README descreve como "eCO2 fixo em 400".
    if (getENS160Status() != STATUS_OPERACAO_NORMAL) {
        return 400;
    }

    // ── A compensação do TODO 4 ──────────────────────────────────────────
    // Sem ela, o sensor real erra de um jeito discreto. O falso erra de um
    // jeito impossível de ignorar, e ainda avisa. Um erro silencioso vira
    // um erro barulhento — que é o objetivo de um teste.
    if (!e.compensado) {
        Serial.println("[Falso] ENS160 lido sem setTempAndHum(): faltou a "
                       "compensacao do TODO 4.");
        return (uint16_t)(e.eco2 + 1500);
    }

    return e.eco2;
}


// ═════════════════════════════════════════════════════════════════════════
//  Controles do teste
// ═════════════════════════════════════════════════════════════════════════

void falso_aht_definir(float temperatura, float umidade) {
    e.temperatura = temperatura;
    e.umidade     = umidade;
}

void falso_aht_falhar_init(bool falhar)            { e.aht_falha_init = falhar; }
void falso_aht_falhar_proxima_leitura(bool falhar) { e.aht_falha_leitura = falhar; }
bool falso_aht_foi_iniciado()                      { return e.aht_iniciado; }

void falso_ens_falhar_init(bool falhar)  { e.ens_falha_init = falhar; }
void falso_ens_aquecimento_ms(uint32_t ms) { e.ens_aquecimento = ms; }

void falso_ens_definir(uint16_t eco2, uint16_t tvoc, uint8_t aqi) {
    e.eco2 = eco2;
    e.tvoc = tvoc;
    e.aqi  = aqi;
}

bool    falso_ens_foi_iniciado()       { return e.ens_iniciado; }
uint8_t falso_ens_modo_configurado()   { return e.ens_modo; }
bool    falso_ens_foi_compensado()     { return e.compensado; }
float   falso_ens_temperatura_recebida() { return e.temp_compensacao; }
float   falso_ens_umidade_recebida()   { return e.umid_compensacao; }
