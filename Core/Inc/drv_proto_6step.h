#ifndef DRV_PROTO_6STEP_H
#define DRV_PROTO_6STEP_H

#include "drv_proto_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void DRV_6STEP_BuildStart(uint8_t out[6]);
void DRV_6STEP_BuildStop(uint8_t out[6]);
void DRV_6STEP_BuildFaultAck(uint8_t out[6]);
void DRV_6STEP_BuildSpeedRamp(uint16_t rpm, uint16_t duration_ms, uint8_t out[16]);

HAL_StatusTypeDef DRV_6STEP_SendStart(DRV_Common_t *ctx);
HAL_StatusTypeDef DRV_6STEP_SendStop(DRV_Common_t *ctx);
HAL_StatusTypeDef DRV_6STEP_SendFaultAck(DRV_Common_t *ctx);
HAL_StatusTypeDef DRV_6STEP_SendSpeedRamp(DRV_Common_t *ctx, uint16_t rpm, uint16_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif
