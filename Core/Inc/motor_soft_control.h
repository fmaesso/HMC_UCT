#ifndef MOTOR_SOFT_CONTROL_H
#define MOTOR_SOFT_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "drv_proto_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MSC_WARN_NONE = 0,
    MSC_WARN_AGGRESSIVE_STOP,
    MSC_WARN_FAULT_PRESENT,
    MSC_WARN_RPM_NOT_TRACKING
} MSC_Warning_t;

typedef enum
{
    MSC_STATE_IDLE = 0,
    MSC_STATE_RUNNING
} MSC_State_t;

typedef void (*MSC_AggressiveStopCb)(int32_t rpm_user_target,
                                     int32_t rpm_ref,
                                     int32_t rpm_actual);

typedef void (*MSC_RpmTrackingCb)(int32_t rpm_ref,
                                  int32_t rpm_actual,
                                  int32_t rpm_error);

typedef struct
{
    DRV_Common_t *drv;

    int32_t rpm_target_user;
    int32_t rpm_ref;
    int32_t rpm_actual;
    uint32_t fault_actual;

    MSC_State_t state;
    MSC_Warning_t warning;

    uint8_t start_sent;
    uint8_t stop_sent;

    uint8_t block_ramp_this_cycle;

    uint8_t aggressive_lock_active;
    int32_t aggressive_lock_target;

    uint8_t lower_limit_lock_active;
    uint8_t lower_limit_latched;
    int32_t lower_limit_rpm;

    uint8_t decel_window_active;
    int32_t decel_window_start_target;
    uint32_t decel_window_start_ms;

    uint32_t last_send_ms;
    uint32_t last_target_change_ms;
    int32_t last_user_target;
    int32_t last_sent_rpm_ref;

    uint16_t ramp_cmd_time_ms;
    int32_t up_step_10ms;
    int32_t down_step_10ms;
    int32_t down_step_limited_10ms;

    int32_t rpm_min_start;
    int32_t rpm_stop_threshold;

    int32_t rpm_send_deadband;
    uint32_t ramp_send_period_ms;

    int32_t rpm_tracking_warn_threshold;
    uint8_t rpm_tracking_warn_count_needed;
    uint8_t rpm_tracking_warn_count;

    int32_t aggressive_drop_threshold;
    uint32_t aggressive_window_ms;
    int32_t aggressive_limit_step_rpm;

    MSC_AggressiveStopCb on_aggressive_stop;
    MSC_RpmTrackingCb on_rpm_tracking_warn;
    uint8_t aggressive_notified;
    uint8_t rpm_tracking_notified;

    uint8_t first_turn;

} MotorSoftControl_t;

extern MotorSoftControl_t g_msc;

void MSC_Init(MotorSoftControl_t *mc, DRV_Common_t *drv);
void MSC_OnUserTargetChanged(MotorSoftControl_t *mc, int32_t rpm_target);
void MSC_Task10ms(MotorSoftControl_t *mc);

MSC_Warning_t MSC_GetWarning(MotorSoftControl_t *mc);
MSC_State_t MSC_GetState(MotorSoftControl_t *mc);

void MSC_SetAggressiveProtectionLevel(MotorSoftControl_t *mc, uint8_t level);
void MSC_SetAggressiveStopCallback(MotorSoftControl_t *mc, MSC_AggressiveStopCb cb);
void MSC_SetRpmTrackingCallback(MotorSoftControl_t *mc, MSC_RpmTrackingCb cb);
void MSC_SetRampSmoothness(MotorSoftControl_t *mc,
                           int32_t up_step_10ms,
                           int32_t down_step_10ms,
                           uint16_t ramp_cmd_time_ms);
void MSC_SetAggressiveProtectionLimit(MotorSoftControl_t *mc, int32_t limit_step_rpm);

bool MSC_IsAggressiveLockActive(MotorSoftControl_t *mc);
int32_t MSC_GetAggressiveLockTarget(MotorSoftControl_t *mc);
void MSC_ClearAggressiveLock(MotorSoftControl_t *mc);

void MSC_SetLowerLimitRpm(MotorSoftControl_t *mc, int32_t limit_rpm);
bool MSC_IsLowerLimitLockActive(MotorSoftControl_t *mc);
int32_t MSC_GetLowerLimitRpm(MotorSoftControl_t *mc);
void MSC_ClearLowerLimitLock(MotorSoftControl_t *mc);

#ifdef __cplusplus
}
#endif

#endif
