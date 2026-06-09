#ifndef ENCODER_H
#define ENCODER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ================== Tipos ================== */

/* Modos de interpretação do encoder */
typedef enum {
    ENC_MODE_LIST = 0,        /* navegação de lista (1 detent = 1 item) */
    ENC_MODE_RPM,             /* ajuste de RPM com aceleração */
    ENC_MODE_VALUE_LINEAR     /* ajuste linear com passo fixo */
} EncoderMode;

/* Callback opcional para avisar mudança de rpm_target */
typedef void (*EncoderRpmTargetChangedCb)(int32_t rpm_target);

/* Evento bruto do encoder */
typedef struct {
    int16_t  detents;     /* +1 / -1 por detent */
    uint32_t t_ms;        /* timestamp (HAL_GetTick) */
    bool     press;       /* clique curto */
    bool     longp;       /* long press */
} EncoderEvent;

/* Estado de aceleração (usado no modo RPM) */
typedef struct {
    uint32_t last_ms;
} EncoderAccelState;

/* Contexto completo do encoder */
typedef struct {
    EncoderMode mode;

    /* RPM */
    int32_t rpm_target;
    int32_t rpm_min;
    int32_t rpm_max;
    EncoderAccelState rpm_acc;

    /* Lista */
    int32_t list_index;
    int32_t list_min;
    int32_t list_max;

    /* Valor genérico */
    int32_t value;
    int32_t value_min;
    int32_t value_max;
    int32_t value_step;

    int16_t DIV;

    uint8_t SW;
    uint8_t Pre_SW;

    /* NOVO: callback opcional para integração com o controle do motor */
    EncoderRpmTargetChangedCb on_rpm_target_changed;

} EncoderContext;

/* ================== API ================== */

void ExecEncSW(void);

/* Inicialização (TIM1 encoder + botão) */
void Encoder_Init(void);

/* Deve ser chamada pela interrupção do TIM1 */
void Encoder_TIM1_ISR(void);

/* Deve ser chamada pela task periódica (ex.: 10 ms) */
bool Encoder_GetEvent(EncoderEvent *ev);

/* Aplica o evento conforme o modo atual */
void Encoder_ApplyEvent(EncoderContext *ctx, const EncoderEvent *ev);

/* NOVO: registra callback opcional para rpm_target */
void Encoder_SetRpmTargetChangedCallback(EncoderContext *ctx,
                                         EncoderRpmTargetChangedCb cb);

uint8_t EncoderSW_GetAndClearEvent(void);
#ifdef __cplusplus
}
#endif
#endif /* ENCODER_H */
