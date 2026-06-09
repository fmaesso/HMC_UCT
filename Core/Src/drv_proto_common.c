#include "drv_proto_common.h"
#include <string.h>
#include <stdio.h>

static const uint8_t CMD_BEACON[4]   = {0x85, 0xFF, 0xFF, 0xBF};
static const uint8_t CMD_PING[4]     = {0x06, 0x00, 0x00, 0x60};
static const uint8_t BEACON_FOC[4]   = {0x05, 0xDF, 0x07, 0x00};
static const uint8_t BEACON_6STEP[4] = {0x05, 0xC7, 0x01, 0xA0};

static uint8_t aspep_type_nibble(uint8_t b0)
{
    return (uint8_t)(b0 & 0x0F);
}

static uint16_t aspep_payload_len_13b(const uint8_t header4[4])
{
    uint32_t v = (uint32_t)header4[0]
               | ((uint32_t)header4[1] << 8)
               | ((uint32_t)header4[2] << 16)
               | ((uint32_t)header4[3] << 24);
    return (uint16_t)((v >> 4) & 0x1FFF);
}

static bool is_request_with_payload(const uint8_t *data, uint16_t len)
{
    return (len > 4U) && (aspep_type_nibble(data[0]) == 9U);
}

void DRV_Common_DebugLog(DRV_Common_t *ctx, const char *fmt, ...)
{
    char msg[180];
    va_list ap;

    if ((ctx == NULL) || (ctx->cb.on_debug_log == NULL) || (fmt == NULL)) {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    ctx->cb.on_debug_log(msg);
}

static void log_hex_packet(DRV_Common_t *ctx, const char *prefix, const uint8_t *data, uint16_t len)
{
    char line[220];
    char *p = line;
    size_t rem = sizeof(line);
    uint16_t i;
    int n;

    if ((ctx == NULL) || (!ctx->debug_data) || (ctx->cb.on_debug_log == NULL) ||
        (prefix == NULL) || (data == NULL) || (len == 0U)) {
        return;
    }

    n = snprintf(p, rem, "%s", prefix);
    if (n < 0 || (size_t)n >= rem) return;
    p += n; rem -= (size_t)n;

    for (i = 0U; i < len; i++) {
        n = snprintf(p, rem, "%02X%s", data[i], (i + 1U < len) ? " " : "");
        if (n < 0 || (size_t)n >= rem) break;
        p += n; rem -= (size_t)n;
    }

    ctx->cb.on_debug_log(line);
}

static void conn_state_set(DRV_Common_t *ctx, DRV_ConnState_t new_state)
{
    DRV_ConnState_t old_state;

    if (ctx == NULL) return;
    if (ctx->conn_state == new_state) return;

    old_state = ctx->conn_state;
    ctx->conn_state = new_state;

    if (ctx->cb.on_link_changed != NULL) {
        ctx->cb.on_link_changed(ctx, old_state, new_state);
    }
}

static HAL_StatusTypeDef uart_send_atomic(DRV_Common_t *ctx, const uint8_t *data, uint16_t len)
{
    if ((ctx == NULL) || (ctx->huart == NULL) || (data == NULL) || (len == 0U)) {
        return HAL_ERROR;
    }
    return HAL_UART_Transmit(ctx->huart, (uint8_t *)data, len, 200U);
}

static HAL_StatusTypeDef uart_send_with_pause(DRV_Common_t *ctx,
                                              const uint8_t *header4,
                                              const uint8_t *payload,
                                              uint16_t payload_len)
{
    HAL_StatusTypeDef st;

    st = uart_send_atomic(ctx, header4, 4U);
    if (st != HAL_OK) return st;

    HAL_Delay(DRV_INTRA_PACKET_MS);

    return uart_send_atomic(ctx, payload, payload_len);
}

static HAL_StatusTypeDef DRV_Common_SendRawFrame(DRV_Common_t *ctx,
                                                 const uint8_t *data,
                                                 uint16_t len)
{
    HAL_StatusTypeDef st;

    if ((ctx == NULL) || (data == NULL) || (len == 0U)) {
        return HAL_ERROR;
    }

    log_hex_packet(ctx, "TX: ", data, len);

    if (is_request_with_payload(data, len)) {
        st = uart_send_with_pause(ctx, data, &data[4], (uint16_t)(len - 4U));
    } else {
        st = uart_send_atomic(ctx, data, len);
    }

    return st;
}

static void drv_reset_runtime(DRV_Common_t *ctx)
{
    if (ctx == NULL) return;

    ctx->conn_state = DRV_CONN_IDLE;
    ctx->tx_state = DRV_TX_IDLE;
    ctx->tx_kind = DRV_TX_KIND_NONE;

    ctx->aspep_configured = false;
    ctx->motor_on = false;

    ctx->last_fault_reg25 = 0U;
    ctx->last_rpm_reg89 = 0U;

    ctx->read_phase = 0U;
    ctx->read_next_due_ms = 0U;
    ctx->read_pause_until_ms = 0U;

    ctx->beacon_rx = 0U;
    ctx->ping_rx_total = 0U;

    ctx->tx_expect_reg = 0U;
    ctx->rx_len = 0U;
    ctx->rx_dma_old_pos = 0U;
}

void DRV_Common_Init(DRV_Common_t *ctx, UART_HandleTypeDef *huart, const DRV_Callbacks_t *callbacks)
{
    if (ctx == NULL) return;

    memset(ctx, 0, sizeof(*ctx));
    ctx->huart = huart;
    ctx->mode = DRV_MODE_FOC;
    ctx->motor_id = 0U;
    ctx->read_enabled = true;
    ctx->crc_payload_enabled = false;
    ctx->debug_data = false;

    if (callbacks != NULL) {
        ctx->cb = *callbacks;
    } else {
        memset(&ctx->cb, 0, sizeof(ctx->cb));
    }

    memcpy(ctx->expected_beacon, BEACON_FOC, 4U);
    drv_reset_runtime(ctx);
}

void DRV_Common_SetMode(DRV_Common_t *ctx, DRV_Mode_t mode)
{
    if (ctx == NULL) return;
    ctx->mode = mode;
    if (mode == DRV_MODE_6STEP) {
        memcpy(ctx->expected_beacon, BEACON_6STEP, 4U);
    } else {
        memcpy(ctx->expected_beacon, BEACON_FOC, 4U);
    }
}

void DRV_Common_SetMotorId(DRV_Common_t *ctx, uint8_t motor_id)
{
    if (ctx == NULL) return;
    ctx->motor_id = motor_id;
}

void DRV_Common_EnableReads(DRV_Common_t *ctx, bool en)
{
    if (ctx == NULL) return;
    ctx->read_enabled = en;
}

void DRV_Common_SetDebugData(DRV_Common_t *ctx, bool en)
{
    if (ctx == NULL) return;
    ctx->debug_data = en;
}

HAL_StatusTypeDef DRV_Common_StartDmaReception(DRV_Common_t *ctx)
{
    if ((ctx == NULL) || (ctx->huart == NULL)) {
        return HAL_ERROR;
    }

    ctx->rx_dma_old_pos = 0U;

    if (HAL_UARTEx_ReceiveToIdle_DMA(ctx->huart, ctx->rx_dma_buf, DRV_RX_DMA_BUFFER_SIZE) != HAL_OK) {
        return HAL_ERROR;
    }

    if (ctx->huart->hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(ctx->huart->hdmarx, DMA_IT_HT);
    }

    return HAL_OK;
}

void DRV_Common_RxEventCallback(DRV_Common_t *ctx, uint16_t pos)
{
    uint16_t old_pos;
    uint16_t len;

    if (ctx == NULL) return;
    if (pos > DRV_RX_DMA_BUFFER_SIZE) return;

    old_pos = ctx->rx_dma_old_pos;

    if (pos >= old_pos) {
        len = (uint16_t)(pos - old_pos);
        if (len > 0U) {
            DRV_Common_OnRxBytes(ctx, &ctx->rx_dma_buf[old_pos], len);
        }
    } else {
        len = (uint16_t)(DRV_RX_DMA_BUFFER_SIZE - old_pos);
        if (len > 0U) {
            DRV_Common_OnRxBytes(ctx, &ctx->rx_dma_buf[old_pos], len);
        }
        if (pos > 0U) {
            DRV_Common_OnRxBytes(ctx, &ctx->rx_dma_buf[0], pos);
        }
    }

    ctx->rx_dma_old_pos = pos;
}

void DRV_Common_BuildGetRegister(uint8_t motor_id, uint16_t reg_dec, uint8_t out[8])
{
    if (out == NULL) return;

    out[0] = 0x49;
    out[1] = 0x00;
    out[2] = 0x00;
    out[3] = 0x70;
    out[4] = (uint8_t)(0x10U + motor_id);
    out[5] = 0x00;
    out[6] = (uint8_t)(reg_dec & 0xFFU);
    out[7] = (uint8_t)((reg_dec >> 8) & 0xFFU);
}

HAL_StatusTypeDef DRV_Common_SendCommand(DRV_Common_t *ctx,
                                         const uint8_t *data,
                                         uint16_t len,
                                         DRV_TxKind_t kind,
                                         uint16_t expect_reg,
                                         uint32_t timeout_ms)
{
    HAL_StatusTypeDef st;

    if ((ctx == NULL) || (data == NULL) || (len == 0U)) {
        return HAL_ERROR;
    }

    if (!ctx->aspep_configured) {
        return HAL_ERROR;
    }

    if (ctx->tx_state != DRV_TX_IDLE) {
        return HAL_BUSY;
    }

    ctx->tx_state = DRV_TX_WAIT_RESP;
    ctx->tx_kind = kind;
    ctx->tx_expect_reg = expect_reg;
    ctx->tx_deadline_ms = ctx->ms_now + timeout_ms;
    ctx->read_pause_until_ms = ctx->ms_now + DRV_CMD_PAUSE_READS_MS;

    st = DRV_Common_SendRawFrame(ctx, data, len);

    if (st != HAL_OK) {
        ctx->tx_state = DRV_TX_IDLE;
        ctx->tx_kind = DRV_TX_KIND_NONE;
        ctx->tx_expect_reg = 0U;
    }

    return st;
}

void DRV_Common_StartConnection(DRV_Common_t *ctx)
{
    if (ctx == NULL) return;

    drv_reset_runtime(ctx);

    ctx->beacon_rx_base = ctx->beacon_rx;
    ctx->ping_rx_base = ctx->ping_rx_total;

    conn_state_set(ctx, DRV_CONN_WAIT_BEACON_1);
    ctx->conn_deadline_ms = ctx->ms_now + DRV_INIT_TIMEOUT_MS;

    (void)DRV_Common_SendRawFrame(ctx, CMD_BEACON, 4U);
}

static void process_response_packet(DRV_Common_t *ctx, const uint8_t *pkt, uint16_t len)
{
    uint16_t payload_len;
    uint8_t status;
    const uint8_t *payload;
    const uint8_t *data_bytes;
    uint16_t data_len;
    uint32_t val = 0U;
    uint16_t i;

    if ((ctx == NULL) || (pkt == NULL) || (len < 5U)) return;

    payload_len = (uint16_t)(len - 4U);
    payload = &pkt[4];
    status = payload[payload_len - 1U];
    data_bytes = payload;
    data_len = (uint16_t)(payload_len - 1U);

    if (ctx->tx_state != DRV_TX_WAIT_RESP) {
        return;
    }

    if (ctx->tx_kind == DRV_TX_KIND_CMD) {
        ctx->tx_state = DRV_TX_IDLE;
        ctx->tx_kind = DRV_TX_KIND_NONE;
        ctx->tx_expect_reg = 0U;
        return;
    }

    if (ctx->tx_kind == DRV_TX_KIND_READ) {
        if (status == 0U) {
            if (data_len > 4U) data_len = 4U;
            for (i = 0U; i < data_len; i++) {
                val |= ((uint32_t)data_bytes[i]) << (8U * i);
            }

            if (ctx->tx_expect_reg == 25U) {
                uint32_t old_fault = ctx->last_fault_reg25;
                ctx->last_fault_reg25 = val;

                if ((ctx->cb.on_fault_changed != NULL) &&
                    (old_fault != ctx->last_fault_reg25))
                {
                    ctx->cb.on_fault_changed(ctx, old_fault, ctx->last_fault_reg25);
                }
            }
            else if (ctx->tx_expect_reg == 89U) {
                uint32_t old_rpm = ctx->last_rpm_reg89;
                bool old_motor_on = ctx->motor_on;

                ctx->last_rpm_reg89 = val;
                ctx->motor_on = (val > 0U) ? true : false;

                if ((ctx->cb.on_rpm_changed != NULL) &&
                    (old_rpm != ctx->last_rpm_reg89))
                {
                    ctx->cb.on_rpm_changed(ctx, old_rpm, ctx->last_rpm_reg89);
                }

                if ((ctx->cb.on_motor_state_changed != NULL) &&
                    (old_motor_on != ctx->motor_on))
                {
                    ctx->cb.on_motor_state_changed(ctx, ctx->motor_on);
                }
            }
        }

        ctx->tx_state = DRV_TX_IDLE;
        ctx->tx_kind = DRV_TX_KIND_NONE;
        ctx->tx_expect_reg = 0U;
    }
}

void DRV_Common_OnRxBytes(DRV_Common_t *ctx, const uint8_t *data, uint16_t len)
{
    if ((ctx == NULL) || (data == NULL) || (len == 0U)) return;

    if (((uint32_t)ctx->rx_len + len) > DRV_RX_PARSE_BUFFER_SIZE) {
        ctx->rx_len = 0U;
    }

    memcpy(&ctx->rx_parse_buf[ctx->rx_len], data, len);
    ctx->rx_len = (uint16_t)(ctx->rx_len + len);

    while (ctx->rx_len >= 4U) {
        uint8_t hdr[4];
        uint8_t t;
        uint16_t pkt_len;
        uint16_t payload_len;

        memcpy(hdr, ctx->rx_parse_buf, 4U);
        t = aspep_type_nibble(hdr[0]);

        if ((t == 5U) || (t == 6U) || (t == 15U)) {
            uint8_t pkt4[4];
            memcpy(pkt4, ctx->rx_parse_buf, 4U);

            memmove(ctx->rx_parse_buf, &ctx->rx_parse_buf[4], ctx->rx_len - 4U);
            ctx->rx_len = (uint16_t)(ctx->rx_len - 4U);

            log_hex_packet(ctx, "RX: ", pkt4, 4U);

            if (memcmp(pkt4, ctx->expected_beacon, 4U) == 0) {
                ctx->beacon_rx++;
            } else if (t == 6U) {
                ctx->ping_rx_total++;
            }
            continue;
        }

        if ((t == 9U) || (t == 10U)) {
            payload_len = aspep_payload_len_13b(hdr);
            pkt_len = (uint16_t)(4U + payload_len + (ctx->crc_payload_enabled ? 2U : 0U));

            if (ctx->rx_len < pkt_len) {
                break;
            }

            uint8_t pkt[64];
            if (pkt_len > sizeof(pkt)) {
                memmove(ctx->rx_parse_buf, &ctx->rx_parse_buf[1], ctx->rx_len - 1U);
                ctx->rx_len = (uint16_t)(ctx->rx_len - 1U);
                continue;
            }

            memcpy(pkt, ctx->rx_parse_buf, pkt_len);
            memmove(ctx->rx_parse_buf, &ctx->rx_parse_buf[pkt_len], ctx->rx_len - pkt_len);
            ctx->rx_len = (uint16_t)(ctx->rx_len - pkt_len);

            log_hex_packet(ctx, "RX: ", pkt, pkt_len);

            if (t == 10U) {
                process_response_packet(ctx, pkt, pkt_len);
            }
            continue;
        }

        memmove(ctx->rx_parse_buf, &ctx->rx_parse_buf[1], ctx->rx_len - 1U);
        ctx->rx_len = (uint16_t)(ctx->rx_len - 1U);
    }
}

void DRV_Common_Task10ms(DRV_Common_t *ctx)
{
    if (ctx == NULL) return;
    ctx->ms_now += 10U;
}

static void common_read_step(DRV_Common_t *ctx)
{
    uint8_t pkt[8];
    uint32_t period;

    if (ctx == NULL) return;
    if (!ctx->aspep_configured) return;
    if (!ctx->read_enabled) return;
    if (ctx->tx_state != DRV_TX_IDLE) return;
    if (ctx->ms_now < ctx->read_pause_until_ms) return;
    if (ctx->ms_now < ctx->read_next_due_ms) return;

#if (DRV_READ_WHEN_OFF_ENABLE == 0)
    if (!ctx->motor_on) return;
#endif

    if (ctx->read_phase == 0U) {
        DRV_Common_BuildGetRegister(ctx->motor_id, 25U, pkt);
        if (DRV_Common_SendCommand(ctx, pkt, 8U, DRV_TX_KIND_READ, 25U, DRV_READ_TIMEOUT_MS) == HAL_OK) {
            ctx->read_phase = 1U;
        }
    } else {
        DRV_Common_BuildGetRegister(ctx->motor_id, 89U, pkt);
        if (DRV_Common_SendCommand(ctx, pkt, 8U, DRV_TX_KIND_READ, 89U, DRV_READ_TIMEOUT_MS) == HAL_OK) {
            ctx->read_phase = 0U;
        }
    }

    period = ctx->motor_on ? DRV_READ_PERIOD_ON_MS : DRV_READ_PERIOD_OFF_MS;
    ctx->read_next_due_ms = ctx->ms_now + period;
}

void DRV_Common_Task100ms(DRV_Common_t *ctx)
{
    if (ctx == NULL) return;

    if ((ctx->conn_state == DRV_CONN_WAIT_BEACON_1) ||
        (ctx->conn_state == DRV_CONN_WAIT_BEACON_2) ||
        (ctx->conn_state == DRV_CONN_WAIT_PING)) {

        if (ctx->ms_now > ctx->conn_deadline_ms) {
            conn_state_set(ctx, DRV_CONN_FAIL);
            return;
        }

        if (ctx->conn_state == DRV_CONN_WAIT_BEACON_1) {
            if (ctx->beacon_rx >= (ctx->beacon_rx_base + 1U)) {
                (void)DRV_Common_SendRawFrame(ctx, ctx->expected_beacon, 4U);
                conn_state_set(ctx, DRV_CONN_WAIT_BEACON_2);
                ctx->conn_deadline_ms = ctx->ms_now + DRV_INIT_TIMEOUT_MS;
            }
        }
        else if (ctx->conn_state == DRV_CONN_WAIT_BEACON_2) {
            if (ctx->beacon_rx >= (ctx->beacon_rx_base + 2U)) {
                (void)DRV_Common_SendRawFrame(ctx, CMD_PING, 4U);
                conn_state_set(ctx, DRV_CONN_WAIT_PING);
                ctx->conn_deadline_ms = ctx->ms_now + DRV_INIT_TIMEOUT_MS;
            }
        }
        else if (ctx->conn_state == DRV_CONN_WAIT_PING) {
            if (ctx->ping_rx_total >= (ctx->ping_rx_base + 1U)) {
                ctx->aspep_configured = true;
                conn_state_set(ctx, DRV_CONN_DONE);
                ctx->read_next_due_ms = ctx->ms_now + 200U;
            }
        }
    }

    if (ctx->tx_state == DRV_TX_WAIT_RESP) {
        if (ctx->ms_now > ctx->tx_deadline_ms) {
            ctx->tx_state = DRV_TX_IDLE;
            ctx->tx_kind = DRV_TX_KIND_NONE;
            ctx->tx_expect_reg = 0U;
        }
    }

    common_read_step(ctx);
}

uint32_t DRV_Common_GetFaultReg25(const DRV_Common_t *ctx)
{
    if (ctx == NULL) return 0U;
    return ctx->last_fault_reg25;
}

uint32_t DRV_Common_GetRpmReg89(const DRV_Common_t *ctx)
{
    if (ctx == NULL) return 0U;
    return ctx->last_rpm_reg89;
}

bool DRV_Common_IsMotorOn(const DRV_Common_t *ctx)
{
    if (ctx == NULL) return false;
    return ctx->motor_on;
}

bool DRV_Common_IsConfigured(const DRV_Common_t *ctx)
{
    if (ctx == NULL) return false;
    return ctx->aspep_configured;
}
