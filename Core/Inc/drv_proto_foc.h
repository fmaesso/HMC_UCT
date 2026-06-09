#ifndef DRV_PROTO_FOC_H
#define DRV_PROTO_FOC_H

#include "drv_proto_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void DRV_FOC_BuildStart(uint8_t out[6]);
void DRV_FOC_BuildStop(uint8_t out[6]);
void DRV_FOC_BuildFaultAck(uint8_t out[6]);
void DRV_FOC_BuildSpeedRamp(uint16_t rpm, uint16_t duration_ms, uint8_t out[16]);

HAL_StatusTypeDef DRV_FOC_SendStart(DRV_Common_t *ctx);
HAL_StatusTypeDef DRV_FOC_SendStop(DRV_Common_t *ctx);
HAL_StatusTypeDef DRV_FOC_SendFaultAck(DRV_Common_t *ctx);
HAL_StatusTypeDef DRV_FOC_SendSpeedRamp(DRV_Common_t *ctx, uint16_t rpm, uint16_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif
