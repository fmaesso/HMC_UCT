# PROJECT_NOTES.md — Hemocor CEC / UCT

Arquivo de contexto para o Codex continuar a codificação do firmware da UCT do projeto **Hemocor CEC**, um circulador extracorpóreo usado em cirurgias cardíacas. Este arquivo foi criado a partir da análise do projeto `Hemoco_CEC_V1` e deve ser mantido atualizado a cada decisão técnica relevante.

> Atenção: este é um equipamento médico. Toda alteração de firmware que envolva motor, pressão, fluxo, alarmes, energia, interface de operação ou logs deve ser tratada como alteração crítica, com revisão, ensaio em bancada, teste de regressão e validação documentada. Não assumir que o código atual já atende requisitos regulatórios ou de segurança funcional.

---

## 1. Visão geral do sistema

O sistema Hemocor CEC é composto por quatro módulos eletrônicos:

1. **UCT — Unidade de Controle / CPU principal**: foco deste repositório.
2. **Placa Drive**: controle dos motores sem escova.
3. **Placa Sensor de Fluxo**: aquisição/transmissão da vazão.
4. **Placa Fonte de Alimentação**: alimentação, bateria/energia e estados da fonte.

A UCT centraliza a interface gráfica, controle operacional, leitura de sensores locais, comunicação com os módulos dedicados e persistência de configurações/logs.

---

## 2. Hardware da UCT

### MCU e memória

- MCU: **STM32H743IITx**.
- Clock do núcleo: configurado para **480 MHz**.
- Barramento AHB/HCLK: **240 MHz**.
- SDRAM externa dedicada ao display/framebuffer:
  - Base usada pelo LTDC: `0xC0000000`.
  - Tamanho esperado do projeto: **16 MB**.
  - Interface FMC SDRAM 32 bits.
- Flash externa QSPI: **IS25LP128F-JBLE**, 128 Mbit / 16 MB.
  - Primeiros 8 MB: assets TouchGFX em memory-mapped, base `0x90000000`.
  - Segunda metade de 8 MB: filesystem FatFs, offset físico `0x00800000`, endereço memory-mapped correspondente `0x90800000`.

### Display e touch

- Display: 7 polegadas, **1024 x 600 px**.
- LTDC configurado em RGB888.
- Framebuffer LTDC layer 0 em `0xC0000000`.
- Touch capacitivo: driver GT911/Goodix via I2C3, com interrupção em `TS_INT`.

### Controles locais

- Encoder rotativo:
  - `PA8` = `ENCODER_T1` = TIM1_CH1.
  - `PA9` = `ENCODER_T2` = TIM1_CH2.
  - TIM1 em modo encoder TI12.
  - Switch do encoder: `PA15` = `ENCODER_SW`, EXTI15_10.
  - Uso atual: ajuste de RPM e entrada na tela de configuração.

### Sensores locais

- Pressão: 2 sensores **SSCDANN015PDAA5** ligados ao ADC1.
  - Código atual usa `ADC_CHANNEL_7` e `ADC_CHANNEL_17` em `Core/Src/adc.c`.
  - ADC1 em 16 bits, scan de 2 canais, DMA circular, oversampling 64x com right shift 2.
  - Conversão atual em `Core/Src/press.c`, usando:
    - `ADC_MAX_VAL = 1048575.0f`
    - `VREF = 3.0f`
    - `GAIN_AMPOP = 0.3905f`
    - `PSI_TO_MMHG = 51.7149f`
  - O zero é calibrado automaticamente nas primeiras amostras, por sensor.
- Temperatura interna do gabinete: sensor TMP1075/TMP75 via I2C3.
  - Código atual está em `Core/Src/TMP75.c` / `Core/Inc/Tmp75.h`.
  - Endereço definido: `TMP75_ADDR = 0x90` no formato HAL 8-bit address.

### Buzzer e alarmes sonoros

- O briefing do hardware menciona buzzer médico **SBS12M1PC** com 3 pinos dedicados.
- No código analisado não foi encontrado driver específico para o buzzer nem labels de GPIO correspondentes no `.ioc`.
- Antes de implementar alarmes sonoros, definir no esquemático:
  - pinos exatos do MCU;
  - função de cada pino do SBS12M1PC;
  - lógica ativa/inativa;
  - prioridade dos alarmes;
  - padrão sonoro para alarmes baixo/médio/alto.

### Watchdog externo, supervisor de reset e pinos de HOLD

Adicionar ao hardware da UCT o supervisor de reset **TPS3820-33DBVT**.

Sinais definidos:

- `PD4` = `WDI`: saída da UCT para alimentar/pulsar a entrada watchdog do TPS3820-33DBVT.
- `PE2` = `HOLD_PIN`: pino de HOLD associado à fonte de alimentação.
- `PI8` = `MOT_DRV_HOLD`: pino de HOLD da placa Drive de Motor, controlado pela UCT.

Regras de uso:

- O TPS3820-33DBVT deve permanecer alimentado inclusive durante a condição em que a UCT está em espera por HOLD.
- `PD4/WDI` não deve ser apenas alternado em uma interrupção fixa sem critério. O pulso de WDI deve ocorrer somente quando as tarefas críticas estiverem saudáveis:
  - loop principal vivo;
  - comunicação crítica sem travamento;
  - supervisão de alarmes executando;
  - TouchGFX não bloqueando o processamento essencial;
  - máquina de estado de energia/desligamento coerente.
- Se qualquer tarefa crítica travar ou deixar de atualizar seu heartbeat, o firmware deve parar de alimentar `PD4/WDI`, permitindo que o TPS3820-33DBVT reinicie a UCT.
- O watchdog externo deve ser tratado como camada independente do watchdog interno do STM32. Se o IWDG/WWDG interno for usado, documentar a relação entre ambos.
- `PI8/MOT_DRV_HOLD` deve espelhar logicamente a condição de HOLD aplicada à UCT/drive. Quando a UCT estiver em estado de HOLD por decisão da fonte, o drive do motor também deve ser mantido inibido/seguro.

### Comunicação com módulos externos

UARTs inicializadas:

- **UART4** — já usada para a placa Drive.
  - TX: `PA0`.
  - RX: `PC11`.
  - Baud atual: `183175`.
  - Protocolo implementado: ASPEP / MC Protocol adaptado, com modos 6STEP e FOC.
- **UART5** — inicializada, mas sem protocolo de aplicação implementado.
  - RX: `PB12`.
  - TX: `PB13`.
  - Baud atual: `115200`.
  - Candidata para Sensor de Fluxo ou Fonte, conforme esquemático.
- **USART1** — inicializada, mas sem protocolo de aplicação implementado.
  - TX: `PB6`.
  - RX: `PB7`.
  - Baud atual: `115200`.
  - Candidata para Sensor de Fluxo ou Fonte, conforme esquemático.

Documento obrigatório para comunicação com a fonte:

- `PROTOCOLO_COMUNICACAO_FONTE_EQUIPAMENTO.md`
- Este documento deve ser lido antes de implementar `comm_power.c/h`, `pws_if.c/h` ou qualquer parser UART da fonte.
- Ele descreve como a UCT deve consultar/receber as condições da fonte de alimentação e da bateria.
- A implementação da barra superior, da tela Energia/Bateria e da sequência de shutdown/HOLD deve obedecer esse protocolo.

Entradas adicionais configuradas:

- `FREQ_INP1`: `PC7`, TIM3_CH2 input capture.
- `FREQ_INP2`: `PB3`, TIM2_CH2 input capture.
- Estes sinais existem no `.ioc`, mas não foram encontrados algoritmos ativos de medição de fluxo/frequência no código analisado.

---

## 3. Estrutura do projeto

Projeto STM32CubeIDE gerado por CubeMX/TouchGFX.

Pastas principais:

- `Core/Inc` e `Core/Src`: firmware C da UCT, drivers de periféricos e lógica principal.
- `TouchGFX/`: interface gráfica, assets, código gerado e código customizado C++.
- `FATFS/`: FatFs sobre a segunda metade da QSPI.
- `USB_DEVICE/`: USB CDC usado como console/comando de serviço.
- `Drivers/`, `Middlewares/`: HAL, CMSIS, TouchGFX, FatFs etc.
- Linker principal: `STM32H743IITX_FLASH.ld`.
- CubeMX: `Hemoco_CEC_V1.ioc`.

Regras para o Codex:

- Preservar blocos `USER CODE` em arquivos gerados pelo CubeMX.
- Preferir implementar lógica de aplicação em arquivos próprios de `Core/Src` e `Core/Inc`.
- Em TouchGFX, modificar preferencialmente `TouchGFX/gui/src/...` e `TouchGFX/gui/include/...`; evitar editar `TouchGFX/generated/...` manualmente.
- Alterações em `.ioc` podem sobrescrever código gerado. Antes de regenerar, revisar todos os blocos customizados.

---

## 4. Sequência de inicialização atual

Arquivo principal: `Core/Src/main.c`.

Ordem relevante:

1. `MPU_Config()`.
2. Habilita I-Cache e D-Cache.
3. `HAL_Init()`.
4. Habilita backup domain/BKPRAM.
5. `SystemClock_Config()`.
6. `PeriphCommonClock_Config()`.
7. Inicialização CubeMX:
   - GPIO, DMA, CRC, FMC, LTDC, QUADSPI, SPI1, UART4, ADC1, TIM2, RAMECC, RTC, I2C3, SPI5, TIM1, TIM3, UART5, USART1, USB, TIM16, DMA2D, TIM12, FATFS, TouchGFX.
8. Inicialização de aplicação:
   - `DRV_RTC_InitSafe()`.
   - `InitCard()`.
   - `InitHemocor()`.
9. Loop principal:
   - Se recebeu comando USB CDC: `ExecCmd(Serial.Cmd)`.
   - `TimerTask()`.
   - Se GUI não estiver pausada: `MX_TouchGFX_Process()`.

### Observações importantes

- A leitura de `PE2/HOLD_PIN` deve acontecer muito cedo no boot da aplicação, antes de liberar operação normal, comunicação operacional do drive ou telas de operação.
- Se `PE2/HOLD_PIN` estiver ativo durante o boot, a UCT deve entrar em modo HOLD com backlight em `0`, drive inibido via `PI8/MOT_DRV_HOLD` e watchdog externo TPS3820-33DBVT alimentado por uma rotina simples e supervisionada.
- A QSPI é colocada em modo memory-mapped durante `MX_QUADSPI_Init()` por `QSPI_EnterMemoryMapped()`.
- O FatFs precisa sair do modo memory-mapped para ler/escrever a partição de filesystem e depois retornar ao memory-mapped para o TouchGFX.
- `MX_TouchGFX_Process()` roda no loop principal, sem RTOS.
- Temporização básica é feita por flags geradas em `HAL_IncTick()` dentro de `Core/Src/utils.c`.

---

## 5. Mapa de memória e linker

Arquivo: `STM32H743IITX_FLASH.ld`.

Regiões relevantes:

```ld
FLASH  : ORIGIN = 0x08000000, LENGTH = 2048K
QSPI   : ORIGIN = 0x90000000, LENGTH = 8M
QSPIFS : ORIGIN = 0x90800000, LENGTH = 8M
RAM_D1 : ORIGIN = 0x24000000, LENGTH = 512K
RAM_D2 : ORIGIN = 0x30000000, LENGTH = 288K
RAM_D3 : ORIGIN = 0x38000000, LENGTH = 64K
BKPSRAM: ORIGIN = 0x38800000, LENGTH = 4K
```

Seções customizadas:

- `.sram4_section` em `RAM_D3`: usada por `SensoresPressao[]` e `adc_buffer[]`.
- `.BKP_SRAM_Section` em `BKPSRAM`: usada pelo fault log persistente.
- `ExtFlashSection`, `FontFlashSection`, `TextFlashSection` em `QSPI`: assets do TouchGFX.

A partição QSPI de filesystem não é usada pelo linker diretamente; é acessada via driver de disco FatFs em `FATFS/Target/user_diskio.c`.

---

## 6. QSPI / TouchGFX / FatFs

Arquivos principais:

- `Core/Src/quadspi.c`
- `Core/Src/quadspi_is.c`
- `Core/Inc/quadspi_is.h`
- `FATFS/Target/user_diskio.c`
- `Core/Src/arquivos.c`

### Divisão da flash IS25LP128

- `0x000000` a `0x7FFFFF`: imagens/fontes/textos TouchGFX.
- `0x800000` a `0xFFFFFF`: FatFs para configs e logs.

No driver FatFs:

```c
#define FS_SIZE_BYTES        (8u * 1024u * 1024u)
#define FS_FLASH_OFFSET      (8u * 1024u * 1024u)
#define LOGICAL_SECTOR_SIZE  512u
#define PHYS_SECTOR_SIZE     4096u
```

A escrita é feita com read-modify-erase-write de 4 KB para cada setor lógico de 512 B.

### Regras críticas

- Antes de operações indiretas QSPI: sair do memory-mapped com `CSP_QSPI_DisableMemoryMappedMode()` ou `HAL_QSPI_Abort()`.
- Depois de ler/escrever/apagar: retornar com `CSP_QSPI_EnableMemoryMappedMode()`.
- Evitar escrever no filesystem enquanto TouchGFX está desenhando assets da QSPI. Existe `GUI_Pause()`/`GUI_Resume()`, mas atualmente está pouco usado/comentado.
- Se aparecer tela branca, travamento ou BusFault em assets, investigar concorrência QSPI + cache + TouchGFX.

### Arquivos persistidos hoje

- `\config.txt`: configurações de pressão, fluxo, RPM e timers.
- `\cec.txt`: informações de versão, serial, cliente, URL etc.

`InitCard()` monta o FatFs; se falhar, formata com `f_mkfs()` e monta novamente.

---

## 7. Configurações em JSON

Arquivos:

- `Core/Src/configs_json.c`
- `Core/Inc/configs_json.h`
- `Core/Src/cec_json.c`
- `Core/Inc/cec_json.h`
- `Core/Src/arquivos.c`

### Estruturas principais

Em `Core/Inc/cec.h`:

```c
typedef struct {
    float Amin;
    float Amax;
    float Bmin;
    float Bmax;
} cfg_pressao_t;

typedef struct {
    float min;
    float max;
} cfg_range_t;

typedef struct {
    uint32_t tempo;
    char sentido;
    int alarme;
} cfg_timer_t;

typedef struct {
    cfg_pressao_t pressao;
    cfg_range_t fluxo;
    cfg_range_t rpm;
    cfg_timer_t timer[4];
} configs_t;
```

Também existe `cec_info_t` para dados do equipamento, firmware, hardware, serial e cliente.

### Defaults atuais

`configs_defaults()` define:

- Pressão A min/max: `10.5`, `16.5`.
- Pressão B min/max: `120.5`, `140.5`.
- Fluxo min/max: `5.0`, `7.0`.
- RPM min/max: `5.0`, `7.0`.
- 4 timers: `10000`, sentido `'u'`, alarme `1`.

`cec_info_defaults()` define valores de exemplo, incluindo modelo `Hemocor-CEC V001.1`.

### Observação de bug provável

Em `GetConfig()` de `Core/Src/arquivos.c`, após `ReadFile()`, o código faz:

```c
strcat((char *)BufferFile, "}");
```

Isso sugere que a leitura está vindo sem o último `}` ou que houve tentativa de contornar truncamento. O ideal é corrigir a leitura para garantir NUL-termination correta e não concatenar `}` cegamente. Ver `ReadFile()`: atualmente usa `f_gets(dest, finfo.fsize, &fil)`, mas `f_gets` precisa de espaço para `\0` e pode não ler o último byte quando o tamanho é exatamente `finfo.fsize`.

Recomendação: trocar por leitura binária controlada com `f_read()`, checando tamanho máximo do buffer e adicionando `\0` manualmente.

---

## 8. Loop de tempo e tarefas periódicas

Arquivo: `Core/Src/utils.c`.

`HAL_IncTick()` seta flags:

- `BitTempo10ms`
- `BitTempo100ms`
- `BitTempo500ms`
- `BitTempo1s`
- `BitTempo2s`
- `BitTempo5s`

Arquivo: `Core/Src/cec.c`, função `TimerTask()`.

Tarefas atuais:

- A cada 10 ms:
  - Processa fila de eventos do encoder.
  - Se `MotorRun`: `DRV_Common_Task10ms()` e `MSC_Task10ms()`.
  - Executa LEDs.
- A cada 100 ms:
  - Se `MotorRun`: `DRV_Common_Task100ms()`.
  - Executa debounce/temporizador do switch do encoder.
  - Se `enc_ctx.Pre_SW`: chama `ExecEncSW()`.
  - Inicia conversão ADC por DMA para 2 canais.
- A cada 500 ms:
  - `VideoRefresh = 1`.
- A cada 2 s:
  - Se motor ainda não inicializado: chama `InitMotor()` e seta `MotorRun = 1`.

Timers de operação:

- `RunShowTimers()` é chamado pelo callback RTC wakeup `HAL_RTCEx_WakeUpTimerEventCallback()`.
- `ShowTimers` contém contadores, alarmes e modo cronômetro/temporizador.

---

## 9. Encoder e controle de RPM

Arquivos:

- `Core/Src/encoder.c`
- `Core/Inc/encoder.h`
- `Core/Src/motor_soft_control.c`

### Encoder

- TIM1 em modo encoder.
- ISR: `HAL_TIM_IC_CaptureCallback()` em `main.c` chama `Encoder_TIM1_ISR()`.
- Eventos são acumulados em fila circular de 16 eventos.
- `Encoder_ApplyEvent()` ajusta `enc_ctx.rpm_target` quando `enc_ctx.mode == ENC_MODE_RPM`.
- Ganho por velocidade de giro:
  - lento: 10 RPM por detent;
  - médio: 50 RPM por detent;
  - rápido: 100 RPM por detent.
- Se subir a partir de menos de 50 RPM, o delta inicial vira 100 RPM.

### Switch do encoder

- `ExecEncSW()` implementa detecção/debounce/pressão via `Timers.TimerCliqueEncoder`.
- `EncoderSW_GetAndClearEvent()` é lido no `Model::tick()` do TouchGFX.
- Na tela principal, o presenter chama `gotoConfigScreen()` quando recebe o evento.

### Controle suave do motor

Arquivo: `Core/Src/motor_soft_control.c`.

- O encoder não manda RPM diretamente para o drive; ele atualiza o alvo do usuário.
- `MSC_OnUserTargetChanged()` recebe novo alvo.
- `MSC_Task10ms()` cria uma referência suave `rpm_ref` e envia rampas ao drive com deadband/período.
- Proteções atuais:
  - Bloqueio de queda agressiva de RPM (`aggressive_lock_active`).
  - Limite inferior de RPM (`lower_limit_lock_active`) configurado hoje para 1500 RPM em `InitMotor()`.
  - Aviso se RPM real não acompanha referência (`MSC_WARN_RPM_NOT_TRACKING`).
  - Se existe fault do drive, alvo permitido vira zero.

---

## 10. Comunicação com a placa Drive

Arquivos:

- `Core/Src/drv_motor_if.c`
- `Core/Src/drv_proto_common.c`
- `Core/Src/drv_proto_6step.c`
- `Core/Src/drv_proto_foc.c`
- Headers correspondentes em `Core/Inc`.

### Estado atual

- `InitMotor()` configura `g_drv` em **modo 6STEP**:

```c
DRV_Common_SetMode(&g_drv, DRV_MODE_6STEP);
DRV_Common_SetMotorId(&g_drv, 0);
```

- RX UART4 atual é por interrupção byte a byte:
  - `HAL_UART_RxCpltCallback()` chama `DRV_Common_OnRxBytes(&g_drv, RX_Buf_uart4, 1)`.
  - Rearma `HAL_UART_Receive_IT()`.
- Existe código em `drv_proto_common.c` para `ReceiveToIdle_DMA`, mas não está sendo usado no fluxo atual.

### Handshake / protocolo

O protocolo implementa handshake ASPEP/MC Protocol:

- Comando beacon TX: `85 FF FF BF`.
- Beacon esperado FOC: `05 DF 07 00`.
- Beacon esperado 6STEP: `05 C7 01 A0`.
- Ping TX: `06 00 00 60`.
- Estado de conexão:
  - `DRV_CONN_IDLE`
  - `DRV_CONN_WAIT_BEACON_1`
  - `DRV_CONN_WAIT_BEACON_2`
  - `DRV_CONN_WAIT_PING`
  - `DRV_CONN_DONE`
  - `DRV_CONN_FAIL`

Leituras periódicas:

- Registro 25: fault.
- Registro 89: RPM.

### Comandos implementados

Comandos comuns de motor:

- Start: `{0x29,0x00,0x00,0xE0,0x19,0x00}`.
- Stop: `{0x29,0x00,0x00,0xE0,0x21,0x00}`.
- Fault ack: `{0x29,0x00,0x00,0xE0,0x39,0x00}`.
- Speed ramp: frame de 16 bytes; formato difere entre FOC e 6STEP.

No 6STEP, `DRV_6STEP_BuildSpeedRamp()` coloca:

- bytes 10-11: RPM little endian;
- bytes 14-15: duração little endian;
- bytes 8-9 fixos em `0x06, 0x00`.

---

## 10A. Fonte de alimentação, bateria, HOLD e desligamento controlado

A comunicação entre a UCT e a placa Fonte deve ser implementada conforme o documento **`PROTOCOLO_COMUNICACAO_FONTE_EQUIPAMENTO.md`**. Esse documento deve ser considerado fonte primária para formato de pacotes, comandos, estados, timeouts e interpretação dos dados da bateria/fonte.

### Funções esperadas da comunicação com a fonte

A UCT deve obter da fonte, no mínimo:

- presença de rede elétrica;
- operação em rede ou bateria;
- porcentagem de bateria;
- autonomia estimada em bateria;
- estado da fonte;
- estado de carga da bateria;
- falhas da fonte;
- dados completos da bateria para a tela Energia/Bateria;
- versão de firmware da fonte, para apresentação na tela inicial.

Esses dados devem alimentar:

- barra superior global;
- tela Energia/Bateria;
- gerenciador de alarmes;
- logs de eventos/ciclos;
- sequência de desligamento controlado.

### Conceito do botão ON/OFF lógico

O botão ON/OFF do equipamento é puramente lógico para a fonte. Ele informa à fonte que a UCT deve ser desligada, mas não significa necessariamente que a fonte deve cortar imediatamente toda a energia.

Existe uma condição especial em que:

- a UCT deve sair de operação;
- o display deve apagar;
- o drive de motor deve ser mantido em estado seguro/inibido;
- a fonte deve continuar energizada para carregar a bateria.

Nessa condição, a fonte mantém a alimentação enquanto a carga da bateria continua. Ao terminar a carga, a fonte pode desligar sua saída e, consequentemente, a UCT será desligada de fato.

### Papel do `PE2/HOLD_PIN`

`PE2` é o `HOLD_PIN` usado pela lógica de HOLD da fonte/UCT.

Regra de boot:

1. Após reset ou energização, a UCT deve ler `PE2/HOLD_PIN` logo no início da inicialização, antes de liberar operação normal.
2. Se `PE2/HOLD_PIN == 0`, a inicialização segue normalmente.
3. Se `PE2/HOLD_PIN == 1`, a UCT não deve iniciar operação normal. Deve entrar em estado de HOLD aguardando o pino baixar.
4. Enquanto `PE2/HOLD_PIN == 1`:
   - luminosidade do display deve ser ajustada para `0`;
   - TouchGFX não deve apresentar a tela operacional normal;
   - motor/drive deve permanecer inibido;
   - alarmes clínicos operacionais não devem ser iniciados como se o equipamento estivesse em uso;
   - o TPS3820-33DBVT deve continuar alimentado;
   - o firmware deve continuar alimentando `PD4/WDI` de forma controlada, desde que a rotina de HOLD esteja saudável;
   - a UCT deve permanecer em loop simples e robusto aguardando `PE2/HOLD_PIN` voltar para `0`.
5. Quando `PE2/HOLD_PIN` voltar para `0`, a UCT pode continuar ou reiniciar a sequência de boot normal. A estratégia preferencial é reiniciar a UCT para garantir estado limpo.

### Papel do `PI8/MOT_DRV_HOLD`

`PI8` é o `MOT_DRV_HOLD`, usado para controlar a condição de HOLD da placa Drive de Motor.

Regras:

- `PI8/MOT_DRV_HOLD` deve espelhar o estado lógico de HOLD aplicado à UCT.
- Se a UCT estiver em HOLD, a placa Drive também deve ficar em HOLD/inibida.
- Antes de enviar shutdown para a fonte, a UCT deve comandar o motor para estado seguro e garantir que `PI8/MOT_DRV_HOLD` foi aplicado conforme a arquitetura elétrica.
- Ao sair do HOLD, a placa Drive só deve ser liberada depois da inicialização normal, comunicação validada e ausência de falhas impeditivas.

### Sequência de shutdown controlado

Sequência recomendada:

1. Operador aciona o botão ON/OFF lógico ou a UCT recebe condição de desligamento.
2. UCT registra evento de desligamento em log, se o filesystem estiver disponível e for seguro escrever.
3. UCT leva o motor para estado seguro:
   - parar rampas;
   - enviar stop ao drive;
   - aguardar confirmação ou timeout;
   - aplicar `PI8/MOT_DRV_HOLD` conforme necessidade.
4. UCT reduz luminosidade do display para `0` ou inicia transição visual de desligamento.
5. UCT envia comando de shutdown para a fonte usando o protocolo definido em `PROTOCOLO_COMUNICACAO_FONTE_EQUIPAMENTO.md`.
6. UCT aguarda uma janela curta para que a fonte processe o comando.
7. UCT executa `NVIC_SystemReset()` ou mecanismo equivalente de reinicialização controlada.
8. No novo boot:
   - se `PE2/HOLD_PIN == 0`, inicialização normal;
   - se `PE2/HOLD_PIN == 1`, entrar em loop de HOLD com display apagado e drive inibido.

Essa abordagem permite que a fonte decida entre:

- cortar a alimentação da UCT imediatamente; ou
- manter a saída ativa enquanto carrega a bateria, mantendo a UCT em estado inibido por HOLD.

### Estados mínimos sugeridos para a máquina de energia

Criar uma máquina de estado própria para energia/desligamento, por exemplo em `app_power.c/h` ou `pws_if.c/h`:

```c
typedef enum
{
    PWR_STATE_BOOT_CHECK_HOLD = 0,
    PWR_STATE_NORMAL,
    PWR_STATE_ON_BATTERY,
    PWR_STATE_SHUTDOWN_REQUESTED,
    PWR_STATE_WAIT_SOURCE_ACK,
    PWR_STATE_PREPARE_RESET,
    PWR_STATE_HOLD_LOOP,
    PWR_STATE_FAULT
} pwr_state_t;
```

A tela e a barra superior não devem implementar essa lógica diretamente. Elas devem apenas apresentar o estado calculado pela camada de aplicação.

---

## 11. Pressão

Arquivos:

- `Core/Src/press.c`
- `Core/Inc/press.h`

Dados globais:

```c
__attribute__((section(".sram4_section"))) TPressao SensoresPressao[2];
__attribute__((section(".sram4_section"))) adc_type adc_buffer[2];
```

Fluxo atual:

1. `InitHemocor()` calibra ADC e inicia `HAL_ADC_Start_DMA(&hadc1, adc_buffer, 2)`.
2. A cada 100 ms, `TimerTask()` chama novamente `HAL_ADC_Start_DMA()`.
3. `HAL_ADC_ConvCpltCallback()`:
   - Se sensor não calibrado: `Sensor_CalibrateZero()`.
   - Se calibrado: `Sensor_UpdateAverage()`.
   - Para o DMA com `HAL_ADC_Stop_DMA(&hadc1)`.

Conversão atual:

```c
v_adc_inst = raw * (VREF / ADC_MAX_VAL);
psi_inst = ((v_adc_inst - V_Zero_ADC) / 0.3905f) * 7.5f;
mmhg_inst = psi_inst * 51.7149f;
```

Filtro:

- EMA com `alfa = 0.08f`.

Pontos para revisar:

- O fator `7.5f` deve ser verificado contra o sensor real e o condicionamento analógico.
- `Sensor_CalibrateZero()` usa `static acumulador` e `static num_amostras` compartilhados entre os dois sensores. Isso provavelmente mistura os canais. Recomenda-se mover acumulador/contador para `TPressao` ou criar arrays separados por sensor.
- Como é equipamento médico, limites de pressão e alarmes não devem depender apenas da tela; devem existir em camada de controle/supervisão.

---

## 12. Interface TouchGFX

Pastas importantes:

- `TouchGFX/gui/src/model/Model.cpp`
- `TouchGFX/gui/include/gui/model/ModelListener.hpp`
- `TouchGFX/gui/src/runscreen_screen/RunScreenView.cpp`
- `TouchGFX/gui/src/runscreen_screen/RunScreenPresenter.cpp`
- `TouchGFX/gui/src/configscreen_screen/ConfigScreenView.cpp`
- `TouchGFX/gui/src/configscreen_screen/config_funcoes.cpp`
- `TouchGFX/gui/src/containers/...`

### Modelo de dados para a GUI

`Model::tick()`:

- Verifica botão do encoder e avisa `onEncoderButtonPressed()`.
- Chama `ui_getdataPrincipal()` e `ui_getdataBarra()`.
- Envia os dados ao listener/presenter.

Estruturas:

- `TDataBarraSuperior`:
  - data/hora;
  - bateria;
  - energia.
- `TTelaPrincipal`:
  - pressões;
  - pressão média;
  - fluxo;
  - RPM alvo;
  - RPM de retorno do drive;
  - estado/fault do motor;
  - flags de erro de RPM e baixa RPM;
  - timers.

### Tela principal

`RunScreenView::updateTelaPrincipal()` atualiza:

- pressão média;
- pressão 1;
- pressão 2;
- fluxo;
- RPM alvo;
- popups/containers de erro RPM e baixa RPM;
- timers 1 a 3.

### Tela de configuração

Menu atual:

1. Limites de Pressão
2. Limites de Fluxo
3. Limites de RPM
4. Temporizadores
5. Energia e Bateria
6. Data e Hora
7. Relatórios de Ciclos
8. Testes de Hardware
9. Sair

A tela carrega/salva `configs_t` por `getDataConfigTCX()` / `setDataConfigTCX()`.

`SetValoresConfig()` salva valores de pressão, fluxo, RPM e timers em `config.txt`.

### Backlight

Container superior chama:

- `ExecLedDisplayPlus()`
- `ExecLedDisplayMinus()`

Essas funções ajustam `DisplayStatus.Led` e chamam `TPS61165_EasyScale_Set()`.

---

## 13. RTC, data/hora e timers

Arquivos:

- `Core/Src/drv_rtc.c`
- `Core/Src/rtc.c`
- `Core/Src/cec.c`
- `TouchGFX/gui/src/configscreen_screen/ConfigScreenView.cpp`
- `TouchGFX/gui/src/configscreen_screen/config_funcoes.cpp`

Uso atual:

- RTC com LSE.
- `DRV_RTC_InitSafe()` chamado no boot.
- `DRV_RTC_GetDateTime()` e `DRV_RTC_SetDateTime()` usados pela tela de configuração.
- `GetStrTime()` alimenta barra superior.
- RTC Wakeup chama `RunShowTimers()`.

Timers:

- `sentido == 'u'`: cronômetro crescente.
- Caso contrário: temporizador decrescente.
- `ShowTimerAlarmBit[]` indica alarme, mas ainda falta integração completa com camada de alarmes/sonorização.

---

## 14. USB CDC — console de serviço

Arquivo: `USB_DEVICE/App/usbd_cdc_if.c`.

- Comandos terminam com `\r`.
- `Serial.Cmd` é preenchido e `Serial.Ok` é setado.
- `main()` chama `ExecCmd()` quando `Serial.Ok` está ativo.
- `_write()` em `main.c` redireciona `printf` para USB CDC quando conectado.

### Comandos atuais

Comandos interpretados em `Core/Src/cec.c`:

- `FL` — lista arquivos.
- `FR\arquivo` — lê arquivo e imprime conteúdo.
- `FW\arquivo;conteudo` — escreve arquivo.
- `FD\arquivo` — apaga arquivo.
- `FF` — formata filesystem.
- `CF` — restaura configuração de fábrica.
- `CD` — habilita debug de pacotes do drive.
- `Cd` — desabilita debug de pacotes do drive.
- `CI` — inicializa motor/driver.
- `CS` — envia start ao drive.
- `CP` — envia stop ao drive.
- `CL` — envia fault ack ao drive.
- `CR<rpm>;<tempo>` — envia ramp de velocidade. Revisar parse porque o código usa `StrValores[0]` e `StrValores[2]`, mas `GetDados(..., 2)` separa por `;=` e normalmente geraria `StrValores[0]` e `StrValores[1]`.
- `CC` — limpa bloqueio de queda agressiva.
- `EC` — limpa fault log e imprime.
- `ES` — imprime fault log.
- `DL<n>` — ajusta backlight via TPS61165.
- `m<n>` — dump da flash mapeada em `0x90800000`.
- `W<n>` — teste de escrita na flash em `0x800000`.
- `T` — lê temperatura.
- `I` — scan I2C.
- `Z` — apaga chip QSPI inteiro. Perigoso: apaga assets TouchGFX e FS.
- `J` — habilita memory-mapped QSPI.
- `r` — reset via `NVIC_SystemReset()`.

Risco atual:

- `CDC_Receive_HS()` não valida limite de `Serial.Quant` para `Serial.Cmd[256]`. Comando muito longo pode causar overflow. Corrigir antes de uso intensivo.
- Comandos destrutivos (`Z`, `FF`, `W`) devem ser protegidos por modo técnico/senha/compilação debug.

---

## 15. Fault log em BKPSRAM

Arquivos:

- `Core/Src/fault_log.c`
- `Core/Inc/fault_log.h`
- `Core/Src/stm32h7xx_it.c`
- Linker: `.BKP_SRAM_Section` em `0x38800000`.

Comportamento:

- HardFault, MemManage, BusFault e UsageFault salvam frame de pilha e registradores SCB em BKPSRAM.
- Após salvar, chama `NVIC_SystemReset()`.
- O log sobrevive ao reset, se backup domain estiver preservado.
- `FaultLog_Print()` imprime resumo via USB CDC.
- `FaultLog_Clear()` limpa registro.

Comandos USB relacionados:

- `ES`: imprime.
- `EC`: limpa e imprime.

---

## 16. LEDs e backlight

Arquivos:

- `Core/Src/leds.c`
- `Core/Src/TPS61165.c`

LEDs:

- `LED_AZUL` → `LED1`, `PG14`.
- `LED_VERMELHO` → `LED2`, `PB4`.
- `LedOn()` escreve nível baixo no pino; lógica aparente ativa em baixo.

Backlight:

- Controlador TPS61165 via EasyScale em `PD13` (`LCD_LED_LEVEL`).
- `TIM12` é usado para temporização em microssegundos.

Bug provável:

- `Led[2]` é declarado em `main.c` com tamanho 2, mas `ExecLed()` faz `for(int l = 0; l < 3; l++)`. Corrigir para `l < 2` ou aumentar o array se realmente houver terceiro LED.

---

## 17. Pontos não implementados ou incompletos

1. **Watchdog externo TPS3820-33DBVT**
   - Acrescentar pinagem no projeto: `PD4/WDI`.
   - Criar política de heartbeat por tarefas críticas.
   - Não alimentar o WDI se a aplicação estiver travada, se a máquina de energia estiver incoerente ou se tarefas críticas não atualizarem seus heartbeats.
   - Garantir funcionamento durante HOLD com display apagado.

2. **Buzzer médico SBS12M1PC**
   - Falta pinagem no `.ioc`/`main.h` e driver.
   - Falta matriz de alarmes sonoros por prioridade.

2. **Placa Sensor de Fluxo**
   - Interface serial dedicada existe como requisito, mas protocolo não foi encontrado.
   - UI usa fluxo fixo `3.4f` em `ui_getdataPrincipal()`.
   - Implementar driver UART e estrutura de dados real.

3. **Placa Fonte de Alimentação**
   - UARTs disponíveis, mas protocolo não implementado.
   - O documento `PROTOCOLO_COMUNICACAO_FONTE_EQUIPAMENTO.md` deve ser lido e seguido antes de implementar a comunicação.
   - Barra superior mostra bateria fixa `75` e energia fixa `0`.
   - Implementar leitura de estado AC/bateria/carga/falhas, porcentagem de bateria, autonomia, versão de firmware da fonte e dados completos da bateria.
   - Implementar sequência de shutdown controlado com comando para a fonte, reset da UCT e tratamento de `PE2/HOLD_PIN`.
   - Implementar controle de `PI8/MOT_DRV_HOLD` para manter a placa Drive inibida quando a UCT estiver em HOLD.

4. **Alarmes clínicos**
   - `TAlarmes`, enums de origem/nível e configs existem, mas não há máquina de alarmes completa.
   - Implementar supervisão independente da tela: pressão, fluxo, RPM, fonte/bateria, timers e comunicação.

5. **Logs operacionais**
   - Há filesystem, mas não foi encontrado sistema de log de ciclos/eventos clínicos.
   - Criar formato, rotação, timestamps e política de gravação segura na QSPI.

6. **Proteção contra concorrência QSPI/TouchGFX**
   - `GUI_Pause()` existe, mas gravações FatFs não pausam consistentemente a GUI.
   - Escrever logs/configs enquanto tela usa assets externos pode causar falha se não coordenar.

7. **Validação de parsing USB e JSON**
   - Corrigir overflow USB CDC.
   - Corrigir leitura de arquivos e remoção do `strcat("}")`.
   - Validar faixas antes de aceitar configs.

8. **Prioridades de interrupção**
   - Muitas interrupções estão em prioridade 0 no `.ioc`.
   - Para equipamento médico, definir matriz de prioridades: segurança/alarmes/comunicação crítica acima de GUI e console.

---

## 18. Recomendações de próximos passos para o Codex

### Primeiro lote — estabilização sem mudar comportamento externo

0. Confirmar no `.ioc`/esquemático os novos sinais `PD4/WDI`, `PE2/HOLD_PIN` e `PI8/MOT_DRV_HOLD` e criar abstrações em `Core/Inc` e `Core/Src`.
1. Corrigir `ExecLed()` para não acessar fora de `Led[2]`.
2. Corrigir `ReadFile()` para leitura segura com `f_read()` e NUL-termination.
3. Remover necessidade de `strcat("}")` em `GetConfig()`.
4. Corrigir overflow em `CDC_Receive_HS()`.
5. Corrigir parse do comando `CR<rpm>;<tempo>`.
6. Separar acumulador/contador de calibração para os dois sensores de pressão.
7. Adicionar comentários de pinagem e mapa de UARTs em `project_notes.md` sempre que forem confirmados no esquemático.

### Segundo lote — integração dos módulos externos

1. Definir qual UART será Sensor de Fluxo e qual será Fonte.
2. Ler obrigatoriamente `PROTOCOLO_COMUNICACAO_FONTE_EQUIPAMENTO.md` antes de codificar a comunicação da fonte.
3. Criar drivers separados:
   - `flow_if.c/h`
   - `pws_if.c/h` ou `comm_power.c/h`
   - `app_power.c/h` para máquina de energia, shutdown e HOLD
   - `app_watchdog.c/h` para TPS3820-33DBVT e heartbeat crítico
4. Cada driver deve ter:
   - parser robusto;
   - timeout de comunicação;
   - último dado válido;
   - flag de falha de comunicação;
   - função `*_Task10ms()` ou `*_Task100ms()`.
4. Atualizar `ui_data.c` para trocar valores fixos por dados reais.

### Terceiro lote — alarmes

1. Criar camada `alarm_manager.c/h` independente do TouchGFX.
2. Entradas: pressão A/B/média, fluxo, RPM alvo/real, estado drive, fonte/bateria, timers, comunicação.
3. Saídas:
   - estado visual;
   - estado sonoro;
   - log de evento;
   - confirmação/silêncio temporário se permitido.
4. Implementar driver do SBS12M1PC após confirmação de pinagem.

### Quarto lote — logs clínicos e manutenção

1. Criar formato de log com timestamp RTC.
2. Evitar escrita excessiva em QSPI.
3. Implementar rotação de arquivos.
4. Criar comandos USB protegidos para exportar logs.
5. Registrar fault log e reset cause no boot.

---

## 19. Arquivos que o Codex deve conhecer primeiro

Para qualquer mudança, ler nesta ordem:

1. `project_notes.md`
2. `PROTOCOLO_COMUNICACAO_FONTE_EQUIPAMENTO.md` quando a alteração envolver fonte, bateria, barra superior, shutdown, HOLD ou tela Energia/Bateria.
3. `Core/Src/main.c`
3. `Core/Inc/cec.h`
4. `Core/Src/cec.c`
5. `Core/Src/ui_data.c`
6. `TouchGFX/gui/src/model/Model.cpp`
7. `TouchGFX/gui/src/runscreen_screen/RunScreenView.cpp`
8. `TouchGFX/gui/src/configscreen_screen/ConfigScreenView.cpp`
9. `Core/Src/encoder.c`
10. `Core/Src/motor_soft_control.c`
11. `Core/Src/drv_motor_if.c`
12. `Core/Src/drv_proto_common.c`
13. `Core/Src/press.c`
14. `Core/Src/arquivos.c`
15. `FATFS/Target/user_diskio.c`
16. `Core/Src/quadspi.c`
17. `Core/Src/quadspi_is.c`
18. `Core/Src/fault_log.c`
19. `USB_DEVICE/App/usbd_cdc_if.c`
20. `STM32H743IITX_FLASH.ld`

---

## 20. Convenções sugeridas

- Prefixo `DRV_`: comunicação/abstração da placa drive.
- Prefixo `MSC_`: controle suave de motor e regras de RPM.
- Prefixo `FLOW_`: futura placa sensor de fluxo.
- Prefixo `PWS_`: futura placa fonte de alimentação.
- Prefixo `PWR_`: máquina de energia, shutdown, HOLD e estado de alimentação.
- Prefixo `WDG_`: watchdog externo TPS3820-33DBVT e heartbeat de tarefas críticas.
- Prefixo `ALM_`: futuro gerenciador de alarmes.
- Prefixo `UI_` ou `ui_`: dados entregues à interface.
- Evitar variáveis globais novas sem necessidade; quando forem necessárias para TouchGFX/C, documentar no header.
- Toda função chamada de ISR deve ser curta, sem `printf`, sem alocação dinâmica e sem operação de flash/filesystem.
- Não usar `HAL_Delay()` em rotinas críticas de motor, alarmes, comunicação ou GUI, exceto em inicialização controlada.

---

## 21. Estado atual resumido

O projeto já tem base funcional para:

- Boot do STM32H743 com SDRAM, LTDC, TouchGFX e QSPI memory-mapped.
- Interface gráfica básica com tela principal, configuração, barra superior e containers.
- Encoder para ajuste de RPM e entrada na configuração.
- Comunicação inicial com a placa drive via UART4 usando protocolo ASPEP/MC.
- Controle suave de RPM com proteção contra redução agressiva.
- Leitura de dois sensores de pressão por ADC/DMA com filtro EMA.
- FatFs na segunda metade da QSPI para `config.txt` e `cec.txt`.
- Console USB CDC para testes e manutenção.
- Fault log em BKPSRAM.

Ainda faltam ou precisam ser revisados antes de avançar para produto:

- Driver do watchdog externo TPS3820-33DBVT com `PD4/WDI` e política de heartbeat crítico.
- Máquina de energia/desligamento com `PE2/HOLD_PIN`, `PI8/MOT_DRV_HOLD` e comando shutdown para a fonte.
- Driver do buzzer médico e gerenciador de alarmes.
- Integração real com placa de fluxo e placa fonte.
- Logs clínicos/operacionais.
- Correções de segurança de buffer/parsing/filesystem.
- Revisão das constantes de pressão e calibração por canal.
- Revisão das prioridades de interrupção e estratégia de segurança.

---

## 22. Histórico de implementação - 2026-06-09

Primeiro lote de reorganização iniciado a partir das regras deste documento.

### Módulos criados

Arquivos novos em `Core/Inc` e `Core/Src`:

- `board_uct.c/h`: abstração dos pinos de segurança da UCT:
  - `PD4/WDI`;
  - `PE2/HOLD_PIN`;
  - `PI8/MOT_DRV_HOLD`.
- `app_watchdog.c/h`: estrutura inicial do watchdog externo TPS3820-33DBVT com heartbeats de:
  - loop principal;
  - timers periódicos;
  - processamento da UI.
- `app_power.c/h`: máquina inicial de energia/HOLD:
  - leitura de `PE2/HOLD_PIN` no boot de aplicação;
  - entrada em HOLD se `PE2` estiver ativo;
  - backlight em zero;
  - `PI8/MOT_DRV_HOLD` ativo;
  - alimentação controlada do `PD4/WDI` durante HOLD;
  - reset controlado quando `PE2` baixar.
- `app_ui_model.c/h`: camada intermediária para dados consumidos pela TouchGFX, preservando a API atual de `ui_data.c`.

### Arquivos integrados

- `Core/Src/main.c`:
  - inicializa `APP_Watchdog_Init()`;
  - inicializa `APP_Power_Init()`;
  - chama `APP_Power_EnterBootHoldLoop()` se HOLD estiver ativo;
  - marca heartbeat do loop principal e da UI.
- `Core/Src/cec.c`:
  - marca heartbeat dos timers;
  - chama `APP_Watchdog_Task10ms()`;
  - chama `APP_Power_Task100ms()`.
- `Core/Src/ui_data.c`:
  - passou a delegar os dados da tela principal e da barra superior para `app_ui_model`.
- `Core/Src/encoder.c`:
  - entrada no modo configuração corrigida para exigir pressão contínua de 4 segundos;
  - qualquer liberação antes de 4 segundos cancela e reinicia a contagem;
  - o evento só dispara uma vez enquanto o botão permanece pressionado;
  - a trava só é liberada depois que o botão é solto.

### Estado funcional

- O controle real de RPM pelo encoder foi preservado.
- A comunicação com a placa Drive foi preservada.
- A TouchGFX não recebeu alterações visuais manuais.
- Fluxo e dados de energia/bateria continuam simulados na camada `app_ui_model`, marcados internamente como simulados para futura substituição por dados reais.
- Build CMake em `build/Debug` compilou com sucesso após as alterações.

### Git e arquivos gerados

O `.gitignore` foi atualizado para ignorar:

- `build/`;
- `/Debug/`;
- artefatos CMake/Ninja;
- `.obj`, `.elf`, `.map`, `.su`, `.bin`, `.hex`;
- caches locais;
- `TouchGFX/build/`;
- caches gerados de fontes/textos do TouchGFX;
- `Middlewares/ST/touchgfx_backup/`.

Isso evita que artefatos de build e caches sejam incluídos no commit.

### Próximos passos recomendados

1. Confirmar em bancada a lógica de long press do encoder.
2. Confirmar em bancada o comportamento de `PE2/HOLD_PIN`, `PI8/MOT_DRV_HOLD` e `PD4/WDI`.
3. Implementar `comm_power.c/h` usando `PROTOCOLO_COMUNICACAO_FONTE_EQUIPAMENTO.md`.
4. Alimentar a barra superior com dados reais da fonte.
5. Criar timeout/validade para todos os dados exibidos na UI.
