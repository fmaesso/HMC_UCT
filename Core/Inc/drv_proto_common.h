#ifndef DRV_PROTO_COMMON_H
#define DRV_PROTO_COMMON_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_RX_DMA_BUFFER_SIZE      256U
#define DRV_RX_PARSE_BUFFER_SIZE   1024U
#define DRV_READ_PERIOD_ON_MS      2000U
#define DRV_READ_PERIOD_OFF_MS     2000U
#define DRV_READ_WHEN_OFF_ENABLE      1U
#define DRV_CMD_PAUSE_READS_MS      600U
#define DRV_INTRA_PACKET_MS           1U
#define DRV_TX_TIMEOUT_MS          1500U
#define DRV_READ_TIMEOUT_MS         900U
#define DRV_INIT_TIMEOUT_MS        5000U

typedef enum {
    DRV_MODE_FOC = 0,
    DRV_MODE_6STEP
} DRV_Mode_t;

typedef enum {
    DRV_CONN_IDLE = 0,
    DRV_CONN_WAIT_BEACON_1,
    DRV_CONN_WAIT_BEACON_2,
    DRV_CONN_WAIT_PING,
    DRV_CONN_DONE,
    DRV_CONN_FAIL
} DRV_ConnState_t;

typedef enum {
    DRV_TX_IDLE = 0,
    DRV_TX_WAIT_RESP
} DRV_TxState_t;

typedef enum {
    DRV_TX_KIND_NONE = 0,
    DRV_TX_KIND_CMD,
    DRV_TX_KIND_READ
} DRV_TxKind_t;

typedef struct DRV_Common_s DRV_Common_t;

typedef void (*DRV_DebugLogCallback_t)(const char *msg);
typedef void (*DRV_LinkChangedCallback_t)(DRV_Common_t *ctx, DRV_ConnState_t old_state, DRV_ConnState_t new_state);
typedef void (*DRV_RpmChangedCallback_t)(DRV_Common_t *ctx, uint32_t old_rpm, uint32_t new_rpm);
typedef void (*DRV_FaultChangedCallback_t)(DRV_Common_t *ctx, uint32_t old_fault, uint32_t new_fault);
typedef void (*DRV_MotorStateChangedCallback_t)(DRV_Common_t *ctx, bool motor_on);

typedef struct {
    DRV_DebugLogCallback_t on_debug_log;
    DRV_LinkChangedCallback_t on_link_changed;
    DRV_RpmChangedCallback_t on_rpm_changed;
    DRV_FaultChangedCallback_t on_fault_changed;
    DRV_MotorStateChangedCallback_t on_motor_state_changed;
} DRV_Callbacks_t;

typedef struct {
    bool beacon1_rx_ok;
    bool echo_tx_done;
    bool beacon2_rx_ok;
    bool ping_tx_done;
    bool ping_rx_ok;
    bool configured;
} DRV_InitStatus_t;

struct DRV_Common_s {
    UART_HandleTypeDef *huart;
    DRV_Mode_t mode;
    DRV_ConnState_t conn_state;
    DRV_TxState_t tx_state;
    DRV_TxKind_t tx_kind;

    uint8_t motor_id;
    uint8_t expected_beacon[4];

    uint8_t rx_dma_buf[DRV_RX_DMA_BUFFER_SIZE];
    uint16_t rx_dma_old_pos;

    uint8_t rx_parse_buf[DRV_RX_PARSE_BUFFER_SIZE];
    uint16_t rx_len;

    uint32_t ms_now;
    uint32_t conn_deadline_ms;
    uint32_t tx_deadline_ms;
    uint32_t read_next_due_ms;
    uint32_t read_pause_until_ms;

    uint32_t beacon_rx;
    uint32_t ping_rx_total;
    uint32_t beacon_rx_base;
    uint32_t ping_rx_base;

    uint16_t tx_expect_reg;
    uint8_t read_phase;

    bool aspep_configured;
    bool read_enabled;
    bool motor_on;
    bool crc_payload_enabled;

    bool debug_data;

    uint32_t last_fault_reg25;
    uint32_t last_rpm_reg89;

    DRV_InitStatus_t init_status;
    DRV_Callbacks_t cb;
};

void DRV_Common_Init(DRV_Common_t *ctx, UART_HandleTypeDef *huart, const DRV_Callbacks_t *callbacks);
void DRV_Common_SetMode(DRV_Common_t *ctx, DRV_Mode_t mode);
void DRV_Common_SetMotorId(DRV_Common_t *ctx, uint8_t motor_id);
void DRV_Common_EnableReads(DRV_Common_t *ctx, bool en);
void DRV_Common_SetDebugData(DRV_Common_t *ctx, bool en);

HAL_StatusTypeDef DRV_Common_StartDmaReception(DRV_Common_t *ctx);
void DRV_Common_RxEventCallback(DRV_Common_t *ctx, uint16_t pos);
void DRV_Common_OnRxBytes(DRV_Common_t *ctx, const uint8_t *data, uint16_t len);
void DRV_Common_OnRxByteIT(DRV_Common_t *ctx, uint8_t byte);

void DRV_Common_Task10ms(DRV_Common_t *ctx);
void DRV_Common_Task100ms(DRV_Common_t *ctx);
void DRV_Common_StartConnection(DRV_Common_t *ctx);

HAL_StatusTypeDef DRV_Common_SendCommand(DRV_Common_t *ctx,
                                         const uint8_t *data,
                                         uint16_t len,
                                         DRV_TxKind_t kind,
                                         uint16_t expect_reg,
                                         uint32_t timeout_ms);

uint32_t DRV_Common_GetFaultReg25(const DRV_Common_t *ctx);
uint32_t DRV_Common_GetRpmReg89(const DRV_Common_t *ctx);
bool DRV_Common_IsMotorOn(const DRV_Common_t *ctx);
bool DRV_Common_IsConfigured(const DRV_Common_t *ctx);
DRV_ConnState_t DRV_Common_GetConnState(const DRV_Common_t *ctx);
void DRV_Common_GetInitStatus(const DRV_Common_t *ctx, DRV_InitStatus_t *out);

void DRV_Common_BuildGetRegister(uint8_t motor_id, uint16_t reg_dec, uint8_t out[8]);
void DRV_Common_DebugLog(DRV_Common_t *ctx, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
