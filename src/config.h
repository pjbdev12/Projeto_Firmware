#pragma once

// ─────────────────────────────────────────────────────────────────────────
// Todos os números ajustáveis do projeto moram AQUI.
//
// Regra prática: se você se pegar digitando um número solto no meio do
// código, ele provavelmente deveria estar neste arquivo, com um nome.
// ─────────────────────────────────────────────────────────────────────────

// ── Sensores I2C: o módulo ENS160 + AHT21 ────────────────────────────────
//
// O AHT21 responde sempre em 0x38 — não há o que configurar.
//
// O ENS160 vem de fábrica em 0x52, mas a maioria dos módulos combinados
// traz o pino de endereço em nível alto, o que o coloca em 0x53. Quem
// decide a questão é o scanner I2C, em dez segundos. Se o scanner achar
// 0x52, troque o número abaixo.
#define ENS160_ENDERECO          0x53

// ── Sensor de partículas: PMS5003 (UART) ─────────────────────────────────
//
// ATENÇÃO à direção dos fios: as linhas se CRUZAM (apostila cap. 18).
// O que sai do sensor entra no ESP32, e vice-versa.
#define PMS_PINO_RX              16          // ESP32 recebe    <- TX do sensor
#define PMS_PINO_TX              17          // ESP32 transmite -> RX do sensor

// 9600 bps é fixo no PMS5003; não é uma escolha nossa.
#define PMS_BAUD                 9600

// Quanto tempo esperar por um quadro do sensor antes de desistir na
// inicialização. Em operação normal ele manda um quadro por segundo.
#define PMS_TIMEOUT_MS           3000UL

// Quanto tempo a ventoinha precisa girar antes de a leitura valer. Enquanto
// não girou o bastante, pm_ler() devolve .ok = false.
//
// Aqui é ZERO porque este firmware é de bancada: a ventoinha liga no
// pm_init() e nunca mais para, então não há o que aguardar. Na Etapa 3, que
// desliga a ventoinha entre um ciclo e outro para poupar o laser, este
// número passa a valer os 30 s reais do PMS5003.
#define PM_AQUECIMENTO_MS        0UL

// ── Média móvel ──────────────────────────────────────────────────────────
// Quantas leituras entram na média móvel de CADA grandeza.
//   Menor → responde rápido, mas o valor oscila mais.
//   Maior → valor mais estável, mas demora a acompanhar mudanças reais.
//
// TODO (experimento da etapa): teste com 5, 10, 15 e 30, anote o que muda,
// escolha um valor e explique a escolha em um comentário aqui embaixo.
#define TAMANHO_HISTORICO        15

// ── Ritmo do programa ────────────────────────────────────────────────────
// De quanto em quanto tempo o loop() lê os sensores.
//
// Por que 1000 ms e não menos? Porque o ENS160 atualiza a saída uma vez por
// segundo. Pedir mais rápido não traz informação nova: só repetiria o mesmo
// valor. Na Etapa 2 este intervalo passa a ser controlado por um timer de
// hardware, em vez de depender da velocidade do laço.
#define INTERVALO_LEITURA_MS     1000UL
