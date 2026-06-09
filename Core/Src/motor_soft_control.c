#include "motor_soft_control.h"
#include "drv_motor_if.h"
#include <string.h>
#include "encoder.h"

MotorSoftControl_t g_msc;
extern EncoderContext enc_ctx;


static int32_t abs_i32(int32_t x)
{
    return (x >= 0) ? x : -x;
}

static int32_t clamp_nonnegative(int32_t x)
{
    return (x < 0) ? 0 : x;
}

static void enter_idle(MotorSoftControl_t *mc)
{
    mc->state = MSC_STATE_IDLE;
    mc->start_sent = 0U;
    mc->stop_sent = 0U;
    mc->rpm_ref = 0;
    mc->last_sent_rpm_ref = -1;
}

static int32_t compute_allowed_target(MotorSoftControl_t *mc, uint32_t now_ms)
{
    int32_t target = mc->rpm_target_user;

    mc->block_ramp_this_cycle = 0U;

    if (mc->fault_actual != 0U)
    {
        mc->warning = MSC_WARN_FAULT_PRESENT;
        mc->aggressive_notified = 0U;
        return 0;
    }

    if (mc->aggressive_lock_active != 0U)
    {
        mc->warning = MSC_WARN_AGGRESSIVE_STOP;
        mc->rpm_target_user = mc->aggressive_lock_target;
        enc_ctx.rpm_target = mc->aggressive_lock_target;
        mc->block_ramp_this_cycle = 1U;
        return mc->rpm_ref;
    }

    if (mc->lower_limit_lock_active != 0U)
    {
        mc->rpm_target_user = mc->lower_limit_rpm;
        enc_ctx.rpm_target = mc->lower_limit_rpm;
        return mc->lower_limit_rpm;
    }

    if ((mc->state == MSC_STATE_RUNNING) &&
        (mc->rpm_actual > 1000) &&
        (mc->decel_window_active != 0U) &&
        ((now_ms - mc->decel_window_start_ms) <= mc->aggressive_window_ms))
    {
        int32_t drop = mc->decel_window_start_target - mc->rpm_target_user;

        if (drop >= mc->aggressive_drop_threshold)
        {
            int32_t limited_target;

            mc->warning = MSC_WARN_AGGRESSIVE_STOP;

            limited_target = mc->decel_window_start_target - mc->aggressive_limit_step_rpm;
            limited_target = clamp_nonnegative(limited_target);

            mc->aggressive_lock_active = 1U;
            mc->aggressive_lock_target = limited_target;
            mc->rpm_target_user = limited_target;
            enc_ctx.rpm_target = limited_target;
            mc->block_ramp_this_cycle = 1U;

            if (mc->aggressive_notified == 0U)
            {
                mc->aggressive_notified = 1U;
                if (mc->on_aggressive_stop != 0)
                {
                    mc->on_aggressive_stop(mc->rpm_target_user,
                                           mc->rpm_ref,
                                           mc->rpm_actual);
                }
            }

            return mc->rpm_ref;
        }
    }

    mc->aggressive_notified = 0U;
    return target;
}

static void update_soft_reference(MotorSoftControl_t *mc, int32_t target_allowed)
{
    int32_t err = target_allowed - mc->rpm_ref;

    if (err > mc->up_step_10ms)
    {
        mc->rpm_ref += mc->up_step_10ms;
    }
    else if (err < -mc->down_step_10ms)
    {
        mc->rpm_ref -= mc->down_step_10ms;
    }
    else
    {
        mc->rpm_ref = target_allowed;
    }

    if (mc->rpm_ref < 0)
    {
        mc->rpm_ref = 0;
    }
}

static void supervise_rpm_tracking(MotorSoftControl_t *mc)
{
    int32_t err;

    if ((mc->state != MSC_STATE_RUNNING) || (mc->rpm_ref <= mc->rpm_min_start))
    {
        mc->rpm_tracking_warn_count = 0U;
        mc->rpm_tracking_notified = 0U;
        if (mc->warning == MSC_WARN_RPM_NOT_TRACKING)
        {
            mc->warning = MSC_WARN_NONE;
        }
        return;
    }

    err = abs_i32(mc->rpm_ref - mc->rpm_actual);

    if (err >= mc->rpm_tracking_warn_threshold)
    {
        if (mc->rpm_tracking_warn_count < 255U)
        {
            mc->rpm_tracking_warn_count++;
        }

        if ((mc->rpm_tracking_warn_count >= mc->rpm_tracking_warn_count_needed) &&
            (mc->rpm_tracking_notified == 0U))
        {
            mc->rpm_tracking_notified = 1U;
            mc->warning = MSC_WARN_RPM_NOT_TRACKING;

            if (mc->on_rpm_tracking_warn != 0)
            {
                mc->on_rpm_tracking_warn(mc->rpm_ref, mc->rpm_actual, err);
            }
        }
    }
    else
    {
        mc->rpm_tracking_warn_count = 0U;
        mc->rpm_tracking_notified = 0U;
        if (mc->warning == MSC_WARN_RPM_NOT_TRACKING)
        {
            mc->warning = MSC_WARN_NONE;
        }
    }
}

static void try_stop_once(MotorSoftControl_t *mc)
{
    if ((mc->start_sent != 0U) && (mc->stop_sent == 0U))
    {
        if (DRV_MotorIf_SendStop(mc->drv) == HAL_OK)
        {
            mc->stop_sent = 1U;
            mc->start_sent = 0U;
            mc->state = MSC_STATE_IDLE;
            mc->rpm_ref = 0;
            mc->last_sent_rpm_ref = -1;
        }
    }
}

void MSC_SetAggressiveProtectionLevel(MotorSoftControl_t *mc, uint8_t level){
    if (mc == 0) return;

    if (level < 1U) level = 1U;
    if (level > 5U) level = 5U;

    switch (level)
    {
        case 1U: /* mais arisco */
            mc->aggressive_drop_threshold = 400;
            mc->aggressive_window_ms = 220U;
            mc->aggressive_limit_step_rpm = 100;
            break;

        case 2U:
            mc->aggressive_drop_threshold = 600;
            mc->aggressive_window_ms = 180U;
            mc->aggressive_limit_step_rpm = 150;
            break;

        case 3U:
            mc->aggressive_drop_threshold = 800;
            mc->aggressive_window_ms = 150U;
            mc->aggressive_limit_step_rpm = 200;
            break;

        case 4U:
            mc->aggressive_drop_threshold = 1000;
            mc->aggressive_window_ms = 130U;
            mc->aggressive_limit_step_rpm = 250;
            break;

        case 5U: /* mais brando */
        default:
            mc->aggressive_drop_threshold = 1200;
            mc->aggressive_window_ms = 110U;
            mc->aggressive_limit_step_rpm = 300;
            break;
    }

//	g_msc.aggressive_drop_threshold = 600;
//    g_msc.aggressive_window_ms = 200;
//    MSC_SetAggressiveProtectionLimit(&g_msc, 100);
}


void MSC_Init(MotorSoftControl_t *mc, DRV_Common_t *drv)
{
    memset(mc, 0, sizeof(*mc));
    mc->drv = drv;

    mc->state = MSC_STATE_IDLE;
    mc->warning = MSC_WARN_NONE;

    mc->rpm_min_start = 100;
    mc->rpm_stop_threshold = 50;

    mc->up_step_10ms = 40;
    mc->down_step_10ms = 25;
    mc->down_step_limited_10ms = 15;
    mc->ramp_cmd_time_ms = 200;

    mc->rpm_send_deadband = 10;
    mc->ramp_send_period_ms = 50U;

    mc->rpm_tracking_warn_threshold = 150;
    mc->rpm_tracking_warn_count_needed = 5U;

    mc->aggressive_drop_threshold = 1000;
    mc->aggressive_window_ms = 150U;
    mc->aggressive_limit_step_rpm = 200;

    mc->aggressive_lock_active = 0U;
    mc->aggressive_lock_target = 0;
    mc->lower_limit_lock_active = 0U;
    mc->lower_limit_latched = 0U;
    mc->lower_limit_rpm = 0;
    mc->decel_window_active = 0U;
    mc->decel_window_start_target = 0;
    mc->decel_window_start_ms = 0U;
    mc->last_sent_rpm_ref = -1;
}

void MSC_SetRampSmoothness(MotorSoftControl_t *mc,
                           int32_t up_step_10ms,
                           int32_t down_step_10ms,
                           uint16_t ramp_cmd_time_ms)
{
    if (mc == 0) return;

    if (up_step_10ms > 0) mc->up_step_10ms = up_step_10ms;
    if (down_step_10ms > 0) mc->down_step_10ms = down_step_10ms;
    if (ramp_cmd_time_ms > 0U) mc->ramp_cmd_time_ms = ramp_cmd_time_ms;
}

void MSC_SetAggressiveProtectionLimit(MotorSoftControl_t *mc, int32_t limit_step_rpm)
{
    if (mc == 0) return;
    if (limit_step_rpm < 0) limit_step_rpm = 0;
    mc->aggressive_limit_step_rpm = limit_step_rpm;
}

void MSC_SetAggressiveStopCallback(MotorSoftControl_t *mc, MSC_AggressiveStopCb cb)
{
    if (mc == 0) return;
    mc->on_aggressive_stop = cb;
}

void MSC_SetRpmTrackingCallback(MotorSoftControl_t *mc, MSC_RpmTrackingCb cb)
{
    if (mc == 0) return;
    mc->on_rpm_tracking_warn = cb;
}

bool MSC_IsAggressiveLockActive(MotorSoftControl_t *mc)
{
    if (mc == 0) return false;
    return (mc->aggressive_lock_active != 0U);
}

int32_t MSC_GetAggressiveLockTarget(MotorSoftControl_t *mc)
{
    if (mc == 0) return 0;
    return mc->aggressive_lock_target;
}

void MSC_ClearAggressiveLock(MotorSoftControl_t *mc)
{
    if (mc == 0) return;
    mc->aggressive_lock_active = 0U;
    mc->aggressive_notified = 0U;
    mc->decel_window_active = 0U;
    if (mc->warning == MSC_WARN_AGGRESSIVE_STOP)
    {
        mc->warning = MSC_WARN_NONE;
    }
}

void MSC_SetLowerLimitRpm(MotorSoftControl_t *mc, int32_t limit_rpm)
{
    if (mc == 0) return;
    if (limit_rpm < 0) limit_rpm = 0;
    mc->lower_limit_rpm = limit_rpm;
}

bool MSC_IsLowerLimitLockActive(MotorSoftControl_t *mc)
{
    if (mc == 0) return false;
    return (mc->lower_limit_lock_active != 0U);
}

int32_t MSC_GetLowerLimitRpm(MotorSoftControl_t *mc)
{
    if (mc == 0) return 0;
    return mc->lower_limit_rpm;
}

void MSC_ClearLowerLimitLock(MotorSoftControl_t *mc)
{
    if (mc == 0) return;
    mc->lower_limit_lock_active = 0U;
}

void MSC_OnUserTargetChanged(MotorSoftControl_t *mc, int32_t rpm_target)
{
    uint32_t now;

    if (mc == 0) return;

    now = HAL_GetTick();
    mc->last_user_target = mc->rpm_target_user;

    if (mc->aggressive_lock_active != 0U)
    {
        mc->rpm_target_user = mc->aggressive_lock_target;
        enc_ctx.rpm_target = mc->aggressive_lock_target;
        mc->last_target_change_ms = now;
        return;
    }

    if (mc->lower_limit_lock_active != 0U)
    {
        mc->rpm_target_user = mc->lower_limit_rpm;
        enc_ctx.rpm_target = mc->lower_limit_rpm;
        mc->last_target_change_ms = now;
        return;
    }

    rpm_target = (rpm_target < 0) ? 0 : rpm_target;

    if ((mc->lower_limit_latched != 0U) && (rpm_target > mc->lower_limit_rpm))
    {
        mc->lower_limit_latched = 0U;
    }

    if (rpm_target < mc->rpm_target_user)
    {
        if (mc->decel_window_active == 0U)
        {
            mc->decel_window_active = 1U;
            mc->decel_window_start_target = mc->rpm_target_user;
            mc->decel_window_start_ms = now;
        }
        else if ((now - mc->decel_window_start_ms) > mc->aggressive_window_ms)
        {
            mc->decel_window_start_target = mc->rpm_target_user;
            mc->decel_window_start_ms = now;
        }

        if ((mc->lower_limit_rpm > 0) &&
            (mc->lower_limit_latched == 0U) &&
            (mc->rpm_target_user > mc->lower_limit_rpm) &&
            (rpm_target < mc->lower_limit_rpm))
        {
            mc->lower_limit_lock_active = 1U;
            mc->lower_limit_latched = 1U;
            mc->rpm_target_user = mc->lower_limit_rpm;
            enc_ctx.rpm_target = mc->lower_limit_rpm;
            mc->last_target_change_ms = now;
            return;
        }
    }
    else
    {
        mc->decel_window_active = 0U;
    }

    mc->rpm_target_user = rpm_target;
    mc->last_target_change_ms = now;
}

void MSC_Task10ms(MotorSoftControl_t *mc)
{
    uint32_t now_ms;
    int32_t target_allowed;
    int32_t old_rpm_ref;
    uint8_t decelerating = 0U;

    if ((mc == 0) || (mc->drv == 0)) return;
    if (!DRV_Common_IsConfigured(mc->drv)) return;

    now_ms = HAL_GetTick();

    if(mc->aggressive_lock_active){
    	enc_ctx.rpm_target = MSC_GetAggressiveLockTarget(&g_msc);
    }
    if(mc->lower_limit_lock_active){
    	enc_ctx.rpm_target = mc->lower_limit_rpm;
    }
    mc->rpm_actual = (int32_t)DRV_Common_GetRpmReg89(mc->drv);
    mc->fault_actual = DRV_Common_GetFaultReg25(mc->drv);

    if ((mc->warning != MSC_WARN_FAULT_PRESENT) &&
        (mc->warning != MSC_WARN_AGGRESSIVE_STOP))
    {
        mc->warning = MSC_WARN_NONE;
    }

    if ((mc->state == MSC_STATE_IDLE) &&
        (mc->rpm_target_user >= mc->rpm_min_start))
    {
//    	if(!mc->first_turn){
//    		DRV_MotorIf_SendSpeedRamp(mc->drv, 200, 300);
//    	    HAL_Delay(200);
//    		mc->first_turn = 1;
//    	}
        if (DRV_MotorIf_SendStart(mc->drv) == HAL_OK)
        {
            mc->start_sent = 1U;
            mc->stop_sent = 0U;
            mc->state = MSC_STATE_RUNNING;
            mc->last_sent_rpm_ref = -1;
        }
        else
        {
            return;
        }
    }

    target_allowed = compute_allowed_target(mc, now_ms);

    old_rpm_ref = mc->rpm_ref;
    update_soft_reference(mc, target_allowed);

    if (mc->rpm_ref < old_rpm_ref)
    {
        decelerating = 1U;
    }

    if (decelerating && (mc->rpm_ref <= mc->rpm_stop_threshold))
    {
        try_stop_once(mc);
        return;
    }

    if (mc->block_ramp_this_cycle != 0U)
    {
        return;
    }

    if ((now_ms - mc->last_send_ms) >= mc->ramp_send_period_ms)
    {
        if ((mc->last_sent_rpm_ref < 0) ||
            (abs_i32(mc->rpm_ref - mc->last_sent_rpm_ref) >= mc->rpm_send_deadband))
        {
            if (DRV_MotorIf_SendSpeedRamp(mc->drv,
                                          (uint16_t)mc->rpm_ref,
                                          mc->ramp_cmd_time_ms) == HAL_OK)
            {
                mc->last_send_ms = now_ms;
                mc->last_sent_rpm_ref = mc->rpm_ref;
            }
        }
    }

    supervise_rpm_tracking(mc);
}

MSC_Warning_t MSC_GetWarning(MotorSoftControl_t *mc)
{
    if (mc == 0) return MSC_WARN_NONE;
    return mc->warning;
}

MSC_State_t MSC_GetState(MotorSoftControl_t *mc)
{
    if (mc == 0) return MSC_STATE_IDLE;
    return mc->state;
}
