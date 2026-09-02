# Etapa 1 — Os sensores

**O roteiro completo desta etapa é o PDF `roteiro-etapa1.pdf`.** Este arquivo
é só o resumo de bolso: o que é o projeto e quais comandos existem.

Esta etapa se faz **inteira no computador**. Não há montagem, não há placa,
não há sensor — os três sensores existem em versão falsa dentro do projeto,
em `src/sim/`, e são eles que respondem ao seu código.

## O que o programa faz quando estiver pronto

Lê três sensores uma vez por segundo e imprime, para cada grandeza, a leitura
crua e a média das últimas N:

```
T  24.4 C (med  24.3) | UR  53.5 % (med  54.0) | eCO2  512 ppm (med    508) | ...
PM1.0   9 | PM2.5  14 (med  14.5) | PM10  22 (med  22.5)  ug/m3  (n = 15)
```

A coluna crua treme; a coluna `med` fica mais quieta. Produzir essa diferença
é o objetivo da etapa.

## Os comandos

| Comando | O que faz |
|---|---|
| `pio run` | Compila os dois ambientes: o do ESP32 e o do PC |
| `pio test -e native -v` | Roda os **34 testes**. O `-v` é obrigatório: sem ele você não vê a explicação das falhas |
| `pio test -e native -v -f test_media` | Roda só um grupo |
| `pio run -e native -t exec` | Roda o firmware inteiro no seu terminal, em tempo real. `Ctrl-C` para sair |
| `pio run -e experimento -t exec` | A bancada de medição da média móvel: dois ensaios controlados, em números. Usada na seção 11 do roteiro |

Compile **antes** de escrever qualquer coisa. O esqueleto já compila do jeito
que está; assim, se der erro depois, você sabe que o erro é seu.

## Mapa dos arquivos

| Arquivo | Estado | O que é |
|---|---|---|
| `src/config.h` | ✅ pronto | Endereços, pinos e números ajustáveis |
| `src/media.h` | ✅ pronto | O *contrato* da média móvel |
| `src/media.cpp` | ⚠️ **TODO 1 e 2** | O buffer circular e o cálculo |
| `src/sensor_amb.h` | ✅ pronto | Contrato dos sensores I²C |
| `src/sensor_amb.cpp` | ⚠️ **TODO 3, 4 e 5** | AHT21 + ENS160 |
| `src/sensor_pm.h` | ✅ pronto | Contrato do sensor de partículas |
| `src/sensor_pm.cpp` | ⚠️ **TODO 6, 7 e 8** | PMS5003 pela UART |
| `src/main.cpp` | ⚠️ **TODO 9** | Junta tudo e imprime |
| `src/sim/` | ✅ pronto | Os sensores falsos e o Arduino do PC |
| `test/` | ✅ pronto | Os 34 testes |
| `src/scanner_i2c.cpp` | ✅ pronto | Ferramenta para quando o hardware chegar |

Você não cria arquivo nenhum e não muda assinatura de função nenhuma.

> ⚠️ Não altere os `printf()` que já vêm prontos no `main.cpp`. O grupo
> `test_ciclo` lê aquelas linhas para conferir o seu ciclo completo — mudar o
> texto quebra a verificação sem quebrar o programa.

## Os 34 testes

| Grupo | Testes | Cobre |
|---|---|---|
| `test_media` | 7 | TODO 1 e 2 |
| `test_sensor_amb` | 10 | TODO 3, 4 e 5 |
| `test_sensor_pm` | 11 | TODO 6, 7 e 8 |
| `test_ciclo` | 6 | TODO 9 |

Com o esqueleto por fazer, o esperado é `26 failed, 8 succeeded`. Os oito que
já passam são casos em que os *placeholders* acertam por acaso.

## Como isso funciona sem sensor

Os arquivos que você escreve são compilados **sem alteração nenhuma** pelo
compilador do PC. Não existe uma segunda cópia deles. O que muda são as
**bibliotecas**: no ambiente `native`, o `platformio.ini` aponta o compilador
para `src/sim/` antes de qualquer outro lugar.

| `#include` do seu código | na placa resolve para | no PC resolve para |
|---|---|---|
| `<Arduino.h>` | núcleo Arduino do ESP32 | `src/sim/host/` |
| `<Wire.h>` | driver I²C do ESP32 | `src/sim/host/` |
| `<Adafruit_AHTX0.h>` | biblioteca da Adafruit | `src/sim/falsas/` |
| `<DFRobot_ENS160.h>` | biblioteca da DFRobot | `src/sim/falsas/` |
| `<PMS.h>` | biblioteca do PMS5003 | **a mesma biblioteca real** |

O seu código não sabe da troca: ele depende do contrato do `.h`, não de quem
o cumpre.

A última linha não é descuido. A biblioteca do PMS5003 só precisa de um
`Stream` e de `millis()`, então roda no PC sem alteração. O que é falso ali é
o outro lado do fio: uma UART que entrega quadros de 32 bytes com cabeçalho e
checksum montados como o sensor faria. Quem os interpreta é o *parser* de
verdade.

**O que um teste verde garante:** que a lógica está certa e que você chamou
as funções certas com os argumentos certos. **O que não garante:** fiação,
tempo e memória no ESP32, detalhes não documentados das bibliotecas reais.
Passar nos 34 testes não prova que o dispositivo funciona — prova que o que
sobrar não é erro de software.

## Ponteiros

Aparecem em **um** arquivo (`media.cpp`) e nas chamadas a ele no `main.cpp`,
onde o `&` responde "em qual das seis médias registrar". Os dois arquivos de
sensor não têm nenhum: `amb_ler()` e `pm_ler()` devolvem a leitura pronta.

```c
LeituraAmbiente amb = amb_ler();
if (amb.ok) { /* amb.temperatura, amb.umidade, amb.gases_validos ... */ }
```

O roteiro tem uma seção curta só sobre isso.

## Entrega

- [ ] `pio run` compila os dois ambientes sem nenhum warning.
- [ ] `pio test -e native` passa nos **34 testes**.
- [ ] `pio run -e experimento -t exec` mediu os quatro valores de N da tabela.
- [ ] `pio run -e native -t exec` imprime uma leitura por segundo, e a média
      de eCO2 fica em 0 durante os 20 s de aquecimento.
- [ ] Tabela do experimento preenchida, com o valor de `TAMANHO_HISTORICO`
      escolhido e justificado em comentário no `config.h`.
- [ ] Resposta à pergunta do N único para grandezas diferentes.
- [ ] Commit no repositório, com mensagem descritiva.
