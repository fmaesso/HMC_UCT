#include "drv_proto_foc.h"
#include <string.h>

void DRV_FOC_BuildStart(uint8_t out[6])
{
    static const uint8_t cmd[6] = {0x29, 0x00, 0x00, 0xE0, 0x19, 0x00};
    memcpy(out, cmd, 6U);
}

void DRV_FOC_BuildStop(uint8_t out[6])
{
    static const uint8_t cmd[6] = {0x29, 0x00, 0x00, 0xE0, 0x21, 0x00};
    memcpy(out, cmd, 6U);
}

void DRV_FOC_BuildFaultAck(uint8_t out[6])
{
    static const uint8_t cmd[6] = {0x29, 0x00, 0x00, 0xE0, 0x39, 0x00};
    memcpy(out, cmd, 6U);
}

void DRV_FOC_BuildSpeedRamp(uint16_t rpm, uint16_t duration_ms, uint8_t out[16])
{
    out[0]  = 0xC9; out[1]  = 0x00; out[2]  = 0x00; out[3]  = 0xC0;
    out[4]  = 0x08; out[5]  = 0x00; out[6]  = 0xA9; out[7]  = 0x01;
    out[8]  = (uint8_t)(duration_ms & 0xFFU);
    out[9]  = (uint8_t)((duration_ms >> 8) & 0xFFU);
    out[10] = (uint8_t)(rpm & 0xFFU);
    out[11] = (uint8_t)((rpm >> 8) & 0xFFU);
    out[12] = 0x00; out[13] = 0x00; out[14] = 0x00; out[15] = 0x00;
}

HAL_StatusTypeDef DRV_FOC_SendStart(DRV_Common_t *ctx)
{
    uint8_t cmd[6];
    DRV_FOC_BuildStart(cmd);
    return DRV_Common_SendCommand(ctx, cmd, 6U, DRV_TX_KIND_CMD, 0U, DRV_TX_TIMEOUT_MS);
}

HAL_StatusTypeDef DRV_FOC_SendStop(DRV_Common_t *ctx)
{
    uint8_t cmd[6];
    DRV_FOC_BuildStop(cmd);
    return DRV_Common_SendCommand(ctx, cmd, 6U, DRV_TX_KIND_CMD, 0U, DRV_TX_TIMEOUT_MS);
}

HAL_StatusTypeDef DRV_FOC_SendFaultAck(DRV_Common_t *ctx)
{
    uint8_t cmd[6];
    DRV_FOC_BuildFaultAck(cmd);
    return DRV_Common_SendCommand(ctx, cmd, 6U, DRV_TX_KIND_CMD, 0U, DRV_TX_TIMEOUT_MS);
}

HAL_StatusTypeDef DRV_FOC_SendSpeedRamp(DRV_Common_t *ctx, uint16_t rpm, uint16_t duration_ms)
{
    uint8_t cmd[16];
    DRV_FOC_BuildSpeedRamp(rpm, duration_ms, cmd);
    return DRV_Common_SendCommand(ctx, cmd, 16U, DRV_TX_KIND_CMD, 0U, DRV_TX_TIMEOUT_MS);
}
