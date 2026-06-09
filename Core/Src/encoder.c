#include "encoder.h"
#include "stm32h7xx_hal.h"
#include "main.h"
#include "cec.h"


/* TIM1 é usado como encoder */
extern TIM_HandleTypeDef htim1;

extern EncoderContext enc_ctx;
extern EncoderEvent ev;

/* ================== Configurações ================== */

/* Aceleração RPM (1 pulso = 1 detent confirmado por você) */
#define RPM_GAIN_SLOW   10
#define RPM_GAIN_MED    50
#define RPM_GAIN_FAST   100

/* thresholds em ms entre detents */
#define RPM_DT_SLOW_MS  120
#define RPM_DT_MED_MS    50

/* Botão: entrada de configuracao por pressao longa de 4 s */
#define ENC_LONGPRESS_MS 4000

/* ================== Estruturas internas ================== */

typedef struct {
    uint16_t last_cnt;
    uint32_t last_ms;
} EncoderIsrState;

static volatile EncoderIsrState g_isr = {0};

/* fila simples de eventos (ring buffer) */
#define ENC_Q_SIZE 16
static volatile uint8_t q_head = 0, q_tail = 0;
static EncoderEvent q[ENC_Q_SIZE];

/* botão */
static uint32_t sw_hold_ms = 0;
static bool sw_last = true;
static bool sw_long_latched = false;

static int16_t det_acc = 0;

extern TTimers Timers;

/* ================== Utilidades ================== */

static inline int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline bool queue_push_isr(const EncoderEvent *e)
{
    uint8_t next = (q_head + 1) % ENC_Q_SIZE;
    if (next == q_tail) return false;
    q[q_head] = *e;
    q_head = next;
    return true;
}

bool Encoder_GetEvent(EncoderEvent *ev)
{
    if (q_tail == q_head) return false;
    *ev = q[q_tail];
    q_tail = (q_tail + 1) % ENC_Q_SIZE;
    return true;
}

/* NOVO: setter do callback de mudança de rpm_target */
void Encoder_SetRpmTargetChangedCallback(EncoderContext *ctx,
                                         EncoderRpmTargetChangedCb cb)
{
    if (!ctx) return;
    ctx->on_rpm_target_changed = cb;
}

/* ================== Inicialização ================== */

void Encoder_Init(void)
{
    HAL_TIM_Encoder_Start_IT(&htim1, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(&htim1, 0);

    g_isr.last_cnt = 0;
    g_isr.last_ms  = HAL_GetTick();

    sw_last = (HAL_GPIO_ReadPin(ENCODER_SW_GPIO_Port, ENCODER_SW_Pin) == GPIO_PIN_SET);
    sw_hold_ms = 0;
    sw_long_latched = false;
    enc_ctx.rpm_max = 4000;
    enc_ctx.value_max = 100;
    enc_ctx.value_min = 100;
    enc_ctx.list_max = 20;
    enc_ctx.DIV = 4;
    enc_ctx.mode = ENC_MODE_RPM;

    /* NOVO: default sem callback */
    enc_ctx.on_rpm_target_changed = 0;
}

/* ================== Interrupção do TIM1 ================== */

void Encoder_TIM1_ISR(void)
{
    uint16_t cnt = (uint16_t)__HAL_TIM_GET_COUNTER(&htim1);
    int16_t delta = (int16_t)(cnt - g_isr.last_cnt);
    if (delta == 0) return;

    g_isr.last_cnt = cnt;

    det_acc += delta;

    /* Ajuste AQUI: 2 ou 4 (comece com 4) */
//    const int16_t DIV = 2;

    while (det_acc >= enc_ctx.DIV) {
        det_acc -= enc_ctx.DIV;
        EncoderEvent ev = {.detents = +1, .t_ms = HAL_GetTick()};
        queue_push_isr(&ev);
    }
    while (det_acc <= -enc_ctx.DIV) {
        det_acc += enc_ctx.DIV;
        EncoderEvent ev = {.detents = -1, .t_ms = HAL_GetTick()};
        queue_push_isr(&ev);
    }
}

/* ================== Aplicação do evento ================== */

static int32_t rpm_gain_from_dt(uint32_t dt_ms)
{
    if (dt_ms > RPM_DT_SLOW_MS) return RPM_GAIN_SLOW;
    if (dt_ms > RPM_DT_MED_MS)  return RPM_GAIN_MED;
    return RPM_GAIN_FAST;
}

void Encoder_ApplyEvent(EncoderContext *ctx, const EncoderEvent *ev)
{
    if (!ctx || !ev) return;

    switch (ctx->mode)
    {
        case ENC_MODE_LIST:
            ctx->list_index = clamp_i32(
                ctx->list_index + ev->detents,
                ctx->list_min,
                ctx->list_max
            );
            break;

        case ENC_MODE_RPM: {
            uint32_t dt = ev->t_ms - ctx->rpm_acc.last_ms;
            int32_t gain;
            int32_t rpm_delta;
            int32_t old_target;

            if (dt == 0 || dt > 1000) dt = 200;
            ctx->rpm_acc.last_ms = ev->t_ms;

            gain = rpm_gain_from_dt(dt);
            rpm_delta = ev->detents * gain;

            old_target = ctx->rpm_target;

        	if((rpm_delta > 0) && (ctx->rpm_target < 50)){
        		rpm_delta = 100;
        	}

            ctx->rpm_target = clamp_i32(
                ctx->rpm_target + rpm_delta,
                ctx->rpm_min,
                ctx->rpm_max
            );
//            ctx->rpm_target += rpm_delta;
//            rpm_delta = 0;

            /* NOVO: avisa quando rpm_target mudar */
            if ((ctx->rpm_target != old_target) &&
                (ctx->on_rpm_target_changed != 0)) {
                ctx->on_rpm_target_changed(ctx->rpm_target);
            }
        } break;

        case ENC_MODE_VALUE_LINEAR:
            ctx->value = clamp_i32(
                ctx->value + ev->detents * ctx->value_step,
                ctx->value_min,
                ctx->value_max
            );
            break;

        default:
            break;
    }
}

void ExecEncSW(void){
	bool pressed = !(ENCODER_DIR);
	uint32_t now = HAL_GetTick();

	if(sw_long_latched){
		if(!pressed){
			Timers.TimerCliqueEncoderCpy = 0;
			Timers.TimerCliqueEncoderBit = 0;
			enc_ctx.Pre_SW = 0;
			sw_long_latched = false;
			sw_hold_ms = 0;
		}
		return;
	}

	if(!pressed){
		Timers.TimerCliqueEncoderCpy = 0;
		Timers.TimerCliqueEncoderBit = 0;
		enc_ctx.Pre_SW = 0;
		sw_hold_ms = 0;
		return;
	}

	if(!enc_ctx.Pre_SW){
		enc_ctx.Pre_SW = 1;
		sw_hold_ms = now;
		Timers.TimerCliqueEncoderCpy = 0;
		Timers.TimerCliqueEncoderBit = 0;
		return;
	}

	if((now - sw_hold_ms) >= ENC_LONGPRESS_MS){
		enc_ctx.SW = 1;
		enc_ctx.Pre_SW = 0;
		sw_long_latched = true;
		Timers.TimerCliqueEncoderCpy = 0;
		Timers.TimerCliqueEncoderBit = 0;
	}
}

uint8_t EncoderSW_GetAndClearEvent(void){
	if(enc_ctx.SW){
		enc_ctx.SW = 0;
		return 1;
	}
	return 0;
}
