#include "app_ui_model.h"
#include "app_power.h"
#include "arquivos.h"
#include "cec.h"
#include "drv_proto_common.h"
#include "drv_rtc.h"
#include "encoder.h"
#include "motor_soft_control.h"
#include <string.h>

extern EncoderContext enc_ctx;
extern DRV_Common_t g_drv;
extern MotorSoftControl_t g_msc;
extern TPressao SensoresPressao[];
extern TShowTimers ShowTimers;

static void fill_principal(TTelaPrincipal *out)
{
    memset(out, 0, sizeof(*out));

    out->Ind_Fluxo = 3.4f;
    out->Ind_Press1 = SensoresPressao[0].Pressao;
    out->Ind_Press2 = SensoresPressao[1].Pressao;
    out->Ind_PressMed = (out->Ind_Press1 + out->Ind_Press2) / 2.0f;
    out->RPM = (int16_t)enc_ctx.rpm_target;
    out->MotorOK = (int16_t)g_drv.last_fault_reg25;
    out->RPMBack = (int16_t)g_drv.last_rpm_reg89;
    out->ErroRPM = g_msc.aggressive_lock_active;
    out->LowRPM = g_msc.lower_limit_lock_active;
    out->Timer1 = ShowTimers.ShowTimerCount[0];
    out->Timer2 = ShowTimers.ShowTimerCount[1];
    out->Timer3 = ShowTimers.ShowTimerCount[2];
    out->Timer4 = ShowTimers.ShowTimerCount[3];
    out->TimerRun1 = ShowTimers.ShowTimerCountRun[0];
    out->TimerRun2 = ShowTimers.ShowTimerCountRun[1];
    out->TimerRun3 = ShowTimers.ShowTimerCountRun[2];
    out->TimerRun4 = ShowTimers.ShowTimerCountRun[3];
}

static void fill_barra(TDataBarraSuperior *out)
{
    memset(out, 0, sizeof(*out));

    out->Ind_Bateria = 75;
    out->Ind_Energia = 0;
    GetStrTime(out->Ind_DataHora);
}

void APP_UI_Model_GetSnapshot(app_ui_snapshot_t *out)
{
    if (out == 0) {
        return;
    }

    fill_principal(&out->principal);
    fill_barra(&out->barra);
    out->flow_quality = APP_UI_DATA_SIMULATED;
    out->power_quality = APP_UI_DATA_SIMULATED;
    out->hold_active = APP_Power_IsHoldActive();
}

void APP_UI_Model_GetPrincipal(TTelaPrincipal *out)
{
    if (out != 0) {
        fill_principal(out);
    }
}

void APP_UI_Model_GetBarra(TDataBarraSuperior *out)
{
    if (out != 0) {
        fill_barra(out);
    }
}
