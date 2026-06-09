#include <string.h>
#include <stdio.h>

#include "drv_proto_common.h"
#include "drv_proto_6step.h"
#include "drv_proto_foc.h"
#include "drv_motor_if.h"
#include "motor_soft_control.h"
#include "encoder.h"
#include "main.h"




extern UART_HandleTypeDef huart4;
extern DRV_Common_t g_drv;

#ifdef USA_CHAT
extern MotorSoftControl_t g_msc;
#else
extern MSC_Context_t g_msc;
#endif

//extern MSC_Context_t g_msc;

extern uint8_t RX_Buf_uart4[];
extern EncoderContext enc_ctx;


static void on_link_changed(DRV_Common_t *ctx, DRV_ConnState_t old_s, DRV_ConnState_t new_s) { (void)ctx; (void)old_s; (void)new_s; }
static void on_rpm_changed(DRV_Common_t *ctx, uint32_t old_rpm, uint32_t new_rpm) { (void)ctx; (void)old_rpm; (void)new_rpm; }
static void on_fault_changed(DRV_Common_t *ctx, uint32_t old_fault, uint32_t new_fault) { (void)ctx; (void)old_fault; (void)new_fault; }
static void on_motor_state_changed(DRV_Common_t *ctx, bool motor_on) { (void)ctx; (void)motor_on; }

void app_debug(const char *msg)
{
    printf("%s\r\n", msg);
}

DRV_Callbacks_t drv_callbacks =
{
    .on_debug_log            = app_debug,
    .on_link_changed         = on_link_changed,
    .on_rpm_changed          = on_rpm_changed,
    .on_fault_changed        = on_fault_changed,
    .on_motor_state_changed  = on_motor_state_changed,
};

void agresive_cb(void){
	printf("Reducao nao permitida\r\n");
}


static void on_aggressive_stop(int32_t rpm_user_target, int32_t rpm_ref, int32_t rpm_actual)
{
    /* Put your display flag/message here */
    /* Example:
       DisplayWarn = WARN_AGGRESSIVE_STOP;
    */
    (void)rpm_user_target;
    (void)rpm_ref;
    (void)rpm_actual;
}

static void EncoderRpmChanged(int32_t rpm_target)
{
    MSC_OnUserTargetChanged(&g_msc, rpm_target);
}

void InitMotor(void)
{
    DRV_Common_Init(&g_drv, &huart4, &drv_callbacks);
    DRV_Common_SetMode(&g_drv, DRV_MODE_6STEP);   /* or DRV_MODE_FOC */
    DRV_Common_SetMotorId(&g_drv, 0);

//    DRV_Common_SetDebugData(&g_drv, true);

    HAL_UART_Receive_IT(&huart4, RX_Buf_uart4, 1);
    DRV_MotorIf_StartConnection(&g_drv);
	DRV_MotorIf_SendSpeedRamp(&g_drv, 200, 100);

    //    DRV_MotorIf_Init(&g_drv);

    MSC_Init(&g_msc, &g_drv);
    MSC_SetRampSmoothness(&g_msc, 30, 20, 500);
    Encoder_SetRpmTargetChangedCallback(&enc_ctx, EncoderRpmChanged);

    MSC_SetAggressiveProtectionLevel(&g_msc, 3);
    MSC_SetAggressiveStopCallback(&g_msc, on_aggressive_stop);
    MSC_SetLowerLimitRpm(&g_msc, 1500);

}

void DRV_MotorIf_Init(DRV_Common_t *ctx)
{
    (void)ctx;
}

HAL_StatusTypeDef DRV_MotorIf_StartConnection(DRV_Common_t *ctx)
{
    if (ctx == NULL)
    {
        return HAL_ERROR;
    }

    DRV_Common_StartConnection(ctx);
    return HAL_OK;
}

HAL_StatusTypeDef DRV_MotorIf_SendStart(DRV_Common_t *ctx)
{
    if (ctx == NULL)
    {
        return HAL_ERROR;
    }

    if (ctx->mode == DRV_MODE_6STEP)
    {
        return DRV_6STEP_SendStart(ctx);
    }

    return DRV_FOC_SendStart(ctx);
}

HAL_StatusTypeDef DRV_MotorIf_SendStop(DRV_Common_t *ctx)
{
    if (ctx == NULL)
    {
        return HAL_ERROR;
    }

    if (ctx->mode == DRV_MODE_6STEP)
    {
        return DRV_6STEP_SendStop(ctx);
    }

    return DRV_FOC_SendStop(ctx);
}

HAL_StatusTypeDef DRV_MotorIf_SendFaultAck(DRV_Common_t *ctx)
{
    if (ctx == NULL)
    {
        return HAL_ERROR;
    }

    if (ctx->mode == DRV_MODE_6STEP)
    {
        return DRV_6STEP_SendFaultAck(ctx);
    }

    return DRV_FOC_SendFaultAck(ctx);
}

HAL_StatusTypeDef DRV_MotorIf_SendSpeedRamp(DRV_Common_t *ctx, uint16_t rpm, uint16_t duration_ms)
{
    if (ctx == NULL)
    {
        return HAL_ERROR;
    }

    if (ctx->mode == DRV_MODE_6STEP)
    {
        return DRV_6STEP_SendSpeedRamp(ctx, rpm, duration_ms);
    }

    return DRV_FOC_SendSpeedRamp(ctx, rpm, duration_ms);
}
