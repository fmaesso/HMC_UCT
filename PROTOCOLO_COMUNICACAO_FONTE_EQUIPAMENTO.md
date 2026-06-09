# Protocolo de Comunicacao entre Equipamento Principal e Fonte

Este documento descreve como o equipamento principal deve se comunicar com a fonte Hemocor-Fonte-V1A / Fonte CEC pela UART HOST.

A fonte e uma fonte/UPS 24 V baseada em BQ25750, STM32G030K6, bateria LiFePO4 8S2P e BMS JBD. A comunicacao com o equipamento principal serve para supervisao, status da fonte, status da bateria e coordenacao segura com o controlador de motor.

## Interface Fisica

```text
UART HOST: USART1 da fonte
Baudrate: 115200
Formato: 8N1
Fluxo: sem controle de fluxo
Terminacao dos comandos: \r, \n ou \r\n
Terminacao das respostas: \r\n
```

A UART BMS e exclusiva da bateria/BMS JBD e nao deve ser usada pelo equipamento principal.

## Objetivo da Comunicacao

O equipamento principal deve usar a UART HOST para:

- verificar se a fonte esta viva;
- obter a versao do firmware da fonte;
- consultar status reduzido em polling frequente;
- informar se o motor esta ligado ou desligado;
- receber a sinalizacao de chave ON/OFF em OFF;
- informar quando o equipamento esta pronto para entrar em HOLD;
- consultar dados completos da fonte e da BMS;
- limpar falhas recuperaveis.

## Comandos de Producao

```text
PING
VER
STAT
STAT0
STAT1
SHUTDWNn
PMS
BAT
CLRFLT
```

## Resumo dos Comandos

| Comando | Funcao |
|---|---|
| `PING` | Teste de comunicacao |
| `VER` | Le nome e versao do firmware da fonte |
| `STAT` | Le status reduzido da fonte |
| `STAT0` | Informa motor desligado e le status reduzido |
| `STAT1` | Informa motor ligado e le status reduzido |
| `SHUTDWNn` | Informa que o equipamento esta pronto para HOLD em `n` segundos |
| `PMS` | Le status completo da fonte e da BMS |
| `BAT` | Le dados detalhados da bateria/BMS |
| `CLRFLT` | Limpa falhas recuperaveis e tenta rearmar a fonte |

## Handshake Basico

### Teste de Comunicacao

Comando:

```text
PING
```

Resposta esperada:

```text
PONG
```

### Versao de Firmware

Comando:

```text
VER
```

Resposta esperada, exemplo:

```text
OK,FW=FONTE_CEC_BQ25750,VER=0.4.0
```

## Polling Normal

Durante operacao normal, o equipamento principal deve consultar a fonte periodicamente.

O comando mais simples e:

```text
STAT
```

Porem o uso recomendado e informar junto o estado do motor:

```text
STAT0
STAT1
```

Significado:

- `STAT0`: motor esta desligado.
- `STAT1`: motor esta ligado.

O estado do motor tem validade temporaria. Se o equipamento parar de enviar `STAT0` ou `STAT1` por mais que `PSU_MOTOR_STATUS_TIMEOUT_MS`, a fonte deve considerar o estado do motor desconhecido e bloquear entrada em HOLD.

## Handshake de Painel OFF e HOLD

A chave ON/OFF do painel entra na fonte pelo PA5/PS_MODE. Essa chave nao corta a alimentacao diretamente. Ela apenas informa a fonte que o usuario colocou o painel em OFF.

Quando o painel for colocado em OFF, a fonte passa a sinalizar essa condicao no `STAT` usando:

```text
SHDN=1
```

Importante: `SHDN=1` nao e ordem para parar o motor. Ele apenas informa que a chave ON/OFF foi colocada em OFF. A decisao de parar o motor continua sendo do operador e da logica propria do equipamento principal.

O equipamento principal pode usar `SHDN=1` para:

- sinalizar ao operador que o painel esta em OFF;
- bloquear novos ciclos, se isso fizer sentido na aplicacao;
- reduzir funcoes auxiliares que nao dependam da decisao do operador;
- preparar a logica interna para uma futura entrada em HOLD.

O sinal fisico de bloqueio da fonte para o equipamento e o PA6/PWS_INT:

```text
PA6 = 0 -> equipamento liberado
PA6 = 1 -> equipamento em HOLD/bloqueado
```

A fonte so coloca PA6 em `1` depois de receber `SHUTDWNn`, aguardar o tempo solicitado e confirmar que o motor esta desligado.

### Sequencia Recomendada

1. Equipamento principal esta operando normalmente.
2. Equipamento envia `STAT0` ou `STAT1` periodicamente.
3. Usuario muda a chave do painel para OFF.
4. Fonte passa a responder `PANEL=0` e `SHDN=1`.
5. Equipamento principal informa continuamente o estado real do motor com `STAT1` ou `STAT0`.
6. Quando o equipamento estiver pronto para ser bloqueado, ele envia `SHUTDWNn`.
7. Se o motor estiver desligado e valido, a fonte aceita o comando.
8. A fonte aguarda `n` segundos e coloca PA6/PWS_INT em `1`, entrando em `POWER_CUT/HOLD`.

Exemplos:

```text
SHUTDWN0
SHUTDWN3
SHUTDWN10
```

Resposta se aceito:

```text
OK,SHUTDWN
```

Resposta se recusado:

```text
ERR,SHUTDWN
```

O comando `SHUTDWNn` so e aceito pela fonte se:

- painel estiver OFF;
- estado do motor estiver valido;
- motor estiver desligado;
- nao houver bloqueio de seguranca.

Se o motor estiver ligado ou desconhecido, a fonte deve recusar o comando.

`STAT0` sozinho nao deve desligar a fonte. Ele apenas informa que o motor esta desligado. O desligamento/HOLD so ocorre depois de `SHUTDWNn` aceito.

## Comando STAT

O `STAT` retorna um status reduzido, adequado para polling frequente.

Formato:

```text
OK,STATE=...,BQ=...,PANEL=...,SHDN=...,OFF_REASON=...,MOTOR=...,MVALID=...,VAC=...,VSYS=...,VBAT=...,IAC=...,IBAT=...,SOC=...,BREM=...,TREM=...,FLT=0x........
```

### Campos do STAT

| Campo | Significado |
|---|---|
| `STATE` | Estado logico da fonte |
| `BQ` | Bitmap de validade/comunicacao do BQ25750 |
| `PANEL` | Estado da chave ON/OFF do painel |
| `SHDN` | Painel OFF sinalizado ao equipamento |
| `OFF_REASON` | Motivo de bloqueio |
| `MOTOR` | Estado do motor informado pelo equipamento |
| `MVALID` | Validade temporal do estado do motor |
| `VAC` | Tensao da entrada em mV |
| `VSYS` | Tensao da saida/sistema em mV |
| `VBAT` | Tensao da bateria em mV |
| `IAC` | Corrente de entrada em mA |
| `IBAT` | Corrente da bateria em mA |
| `SOC` | Carga da bateria em porcentagem |
| `BREM` | Capacidade restante da bateria em mAh |
| `TREM` | Autonomia estimada em minutos |
| `FLT` | Flags de falha em hexadecimal |

### Campo BQ

O campo `BQ` agrupa tres flags:

```text
bit0 = comunicacao I2C com BQ OK
bit1 = status do BQ valido
bit2 = ADC do BQ valido
```

Valores comuns:

```text
BQ=7 -> comunicacao, status e ADC validos
BQ=0 -> BQ sem comunicacao/dados validos
```

### Campo PANEL

```text
PANEL=1 -> chave do painel em ON
PANEL=0 -> chave do painel em OFF
```

### Campo SHDN

```text
SHDN=0 -> equipamento pode operar normalmente
SHDN=1 -> chave ON/OFF do painel esta em OFF
```

### Campo OFF_REASON

Valores previstos:

```text
NONE
MOTOR_ON
MOTOR_UNKNOWN
BAT_CRITICAL
FAULT
```

Significado:

- `NONE`: nao ha bloqueio relevante.
- `MOTOR_ON`: HOLD bloqueado porque o motor esta ligado.
- `MOTOR_UNKNOWN`: HOLD bloqueado porque o estado do motor expirou ou nao foi informado.
- `BAT_CRITICAL`: bloqueio ou corte por bateria critica.
- `FAULT`: bloqueio ou corte por falha.

### Campos MOTOR e MVALID

```text
MOTOR=0  -> equipamento informou motor desligado
MOTOR=1  -> equipamento informou motor ligado
MVALID=1 -> informacao do motor ainda e valida
MVALID=0 -> informacao do motor expirou/desconhecida
```

Se `MVALID=0`, a fonte deve tratar como `MOTOR_UNKNOWN` e bloquear entrada em HOLD.

### Campo IBAT

```text
IBAT > 0 -> bateria carregando
IBAT = 0 -> corrente nula ou abaixo da resolucao/leitura
IBAT < 0 -> bateria descarregando
```

Em baixa carga, a corrente de bateria pode aparecer como zero mesmo com a fonte operando em bateria.

## Exemplos de STAT

### Rede Presente, Painel ON, Motor Desligado

```text
OK,STATE=AC_OK,BQ=7,PANEL=1,SHDN=0,OFF_REASON=NONE,MOTOR=0,MVALID=1,VAC=24472,VSYS=24686,VBAT=26368,IAC=0,IBAT=0,SOC=99,BREM=10840,TREM=0,FLT=0x00000000
```

### Painel OFF, Motor Ligado

```text
OK,STATE=SHUTDOWN_INHIBITED,BQ=7,PANEL=0,SHDN=1,OFF_REASON=MOTOR_ON,MOTOR=1,MVALID=1,VAC=24450,VSYS=24690,VBAT=26360,IAC=0,IBAT=0,SOC=99,BREM=10840,TREM=0,FLT=0x00000000
```

### Painel OFF, Motor Desligado

```text
OK,STATE=SHUTDOWN_REQUEST,BQ=7,PANEL=0,SHDN=1,OFF_REASON=NONE,MOTOR=0,MVALID=1,VAC=24450,VSYS=24690,VBAT=26360,IAC=0,IBAT=0,SOC=99,BREM=10840,TREM=0,FLT=0x00000000
```

### Painel OFF, Motor Desconhecido

```text
OK,STATE=SHUTDOWN_INHIBITED,BQ=7,PANEL=0,SHDN=1,OFF_REASON=MOTOR_UNKNOWN,MOTOR=0,MVALID=0,VAC=24450,VSYS=24690,VBAT=26360,IAC=0,IBAT=0,SOC=99,BREM=10840,TREM=0,FLT=0x00000000
```

### Operacao em Bateria

```text
OK,STATE=BATTERY,BQ=7,PANEL=1,SHDN=0,OFF_REASON=NONE,MOTOR=0,MVALID=1,VAC=0,VSYS=24020,VBAT=25840,IAC=0,IBAT=-3200,SOC=72,BREM=7920,TREM=148,FLT=0x00000000
```

## Estados da Fonte

Estados esperados no campo `STATE`:

| Estado | Significado |
|---|---|
| `BOOT` | Inicializacao |
| `AC_OK` | Entrada presente e VSYS valida |
| `AC_WEAK` | Entrada presente, mas fraca |
| `AC_PRESENT_NO_SYS` | Entrada presente, mas VSYS ainda nao esta valida |
| `BATTERY` | Operacao em bateria/reverse |
| `BATTERY_REVERSE` | Operacao em bateria/reverse, se esse nome estiver no firmware |
| `NO_INPUT` | Sem entrada valida e sem VSYS valida |
| `SHUTDOWN_REQUEST` | Painel OFF, fonte aguardando autorizacao para HOLD |
| `SHUTDOWN_INHIBITED` | Painel OFF, mas HOLD bloqueado |
| `POWER_CUT` | Equipamento em HOLD pela logica da fonte |
| `FAULT` | Falha ativa |
| `BQ_COMM_FAULT` | Falha de comunicacao com BQ25750 |

Se a entrada cair, a bateria estiver presente e `VSYS` estiver valida, o equipamento deve interpretar como operacao em bateria mesmo que `IBAT` esteja momentaneamente zero.

## Comando PMS

O comando `PMS` retorna o snapshot completo da fonte e da BMS.

Comando:

```text
PMS
```

Resposta conceitual:

```text
OK,STATE=AC_OK,BQ=7,AC=1,WEAK=0,REV=0,CHG=1,PANEL=1,SHDN=0,OFF_INH=0,OFF_REASON=NONE,MOTOR=0,MVALID=1,VAC=24472,VSYS=24686,VBAT=26368,IAC=0,IBAT=0,BMS=1,SOC=99,BREM=10840,BFULL=11000,BCYC=0,TREM=0,BPROT=0,BMOS=3,CELLS=8,CMIN=3290,CMAX=3305,CDEL=15,T1=25,T2=24,FLT=0x00000000
```

Campos adicionais do `PMS`:

| Campo | Significado |
|---|---|
| `AC` | Entrada presente |
| `WEAK` | Entrada fraca |
| `REV` | Reverse/bateria ativo |
| `CHG` | Carga ativa |
| `OFF_INH` | Desligamento bloqueado |
| `BMS` | Dados da BMS validos |
| `BFULL` | Capacidade cheia da bateria em mAh |
| `BCYC` | Ciclos da bateria |
| `BPROT` | Flags de protecao da BMS |
| `BMOS` | Estado dos MOSFETs da BMS |
| `CELLS` | Quantidade de celulas lidas |
| `CMIN` | Menor tensao de celula em mV |
| `CMAX` | Maior tensao de celula em mV |
| `CDEL` | Diferenca entre maior e menor celula em mV |
| `T1` | Temperatura 1 em graus Celsius |
| `T2` | Temperatura 2 em graus Celsius |

## Comando BAT

O comando `BAT` retorna dados detalhados da bateria/BMS.

Comando:

```text
BAT
```

Resposta esperada:

```text
OK,BMS=1,VBAT=25840,IBAT=-3200,SOC=72,BREM=7920,BFULL=11000,BCYC=34,TREM=148,BPROT=0,BMOS=2,CELLS=8,CMIN=3220,CMAX=3245,CDEL=25,T1=28,T2=29
```

Campos:

| Campo | Significado |
|---|---|
| `BMS` | Dados da BMS validos |
| `VBAT` | Tensao da bateria em mV |
| `IBAT` | Corrente da bateria em mA |
| `SOC` | Porcentagem de carga |
| `BREM` | Capacidade restante em mAh |
| `BFULL` | Capacidade cheia em mAh |
| `BCYC` | Ciclos |
| `TREM` | Autonomia estimada em minutos |
| `BPROT` | Flags de protecao da BMS |
| `BMOS` | Estado dos MOSFETs da BMS |
| `CELLS` | Quantidade de celulas |
| `CMIN` | Menor tensao de celula em mV |
| `CMAX` | Maior tensao de celula em mV |
| `CDEL` | Diferenca entre maior e menor celula em mV |
| `T1` | Temperatura 1 em graus Celsius |
| `T2` | Temperatura 2 em graus Celsius |

Se a bateria nao estiver presente ou se `VBAT` do BQ indicar bateria ausente, a fonte deve responder `BMS=0` e zerar os dados derivados da bateria.

Exemplo:

```text
OK,BMS=0,VBAT=0,IBAT=0,SOC=0,BREM=0,BFULL=0,BCYC=0,TREM=0,BPROT=0,BMOS=0,CELLS=0,CMIN=0,CMAX=0,CDEL=0,T1=0,T2=0
```

## Comando CLRFLT

Comando:

```text
CLRFLT
```

Funcao:

- limpar falhas recuperaveis;
- tentar rearmar a fonte;
- reexecutar configuracoes criticas quando aplicavel;
- nao mascarar falhas persistentes.

Resposta esperada:

```text
OK,CLRFLT
```

Resposta em caso de erro:

```text
ERR,CLRFLT
```

## Tratamento de Erros

Respostas de erro seguem o formato:

```text
ERR,<COMANDO>
```

Exemplos:

```text
ERR,SHUTDWN
ERR,CLRFLT
ERR,CMD
```

O equipamento principal deve tratar qualquer resposta `ERR` como comando nao executado.

## Regras para o Equipamento Principal

1. Enviar `STAT0` ou `STAT1` periodicamente.
2. Usar `STAT1` sempre que o motor estiver ligado.
3. Usar `STAT0` somente quando o motor estiver realmente desligado.
4. Se receber `SHDN=1`, tratar como indicacao de chave ON/OFF em OFF.
5. Nao desligar abruptamente o motor apenas por perda de comunicacao; usar a propria logica de seguranca do equipamento.
6. Nao depender de `PMS` para polling rapido; usar `STAT`.
7. Usar `BAT` ou `PMS` apenas quando precisar de dados detalhados.
8. Considerar `FLT != 0x00000000` como presenca de flags de falha ou eventos a interpretar.

## Periodicidade Sugerida

Sugestao inicial para o equipamento principal:

```text
STAT0/STAT1: a cada 500 ms a 1000 ms
PING: apenas diagnostico
BAT: a cada 5 s a 30 s, se necessario
PMS: apenas tela de diagnostico/manutencao
VER: uma vez no boot ou em diagnostico
CLRFLT: somente por acao de recuperacao
```

O tempo entre `STAT0/STAT1` deve ser menor que `PSU_MOTOR_STATUS_TIMEOUT_MS`.

## Observacoes Importantes

- A fonte nunca deve receber comandos crus de registrador do BQ como protocolo de producao.
- O equipamento principal nao deve conversar diretamente com a BMS.
- A fonte e a unica responsavel por consolidar status do BQ25750 e da BMS.
- A chave ON/OFF do painel nao e corte direto; ela sinaliza `PANEL=0` e `SHDN=1`.
- Em `POWER_CUT/HOLD`, a saida HOLD PA6/PWS_INT da fonte fica em `1` para manter o equipamento desabilitado.
- Retorno da rede com painel OFF nao deve religar automaticamente o equipamento.
