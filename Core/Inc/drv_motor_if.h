#ifndef DRV_MOTOR_IF_H
#define DRV_MOTOR_IF_H

#include "drv_proto_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void InitMotor(void);
void DRV_MotorIf_Init(DRV_Common_t *ctx);

HAL_StatusTypeDef DRV_MotorIf_StartConnection(DRV_Common_t *ctx);

HAL_StatusTypeDef DRV_MotorIf_SendStart(DRV_Common_t *ctx);
HAL_StatusTypeDef DRV_MotorIf_SendStop(DRV_Common_t *ctx);
HAL_StatusTypeDef DRV_MotorIf_SendFaultAck(DRV_Common_t *ctx);
HAL_StatusTypeDef DRV_MotorIf_SendSpeedRamp(DRV_Common_t *ctx, uint16_t rpm, uint16_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif
