#ifndef INC_APP_POWER_H_
#define INC_APP_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    APP_POWER_STATE_BOOT_CHECK_HOLD = 0,
    APP_POWER_STATE_NORMAL,
    APP_POWER_STATE_SHUTDOWN_REQUESTED,
    APP_POWER_STATE_HOLD_LOOP,
    APP_POWER_STATE_FAULT
} app_power_state_t;

void APP_Power_Init(void);
void APP_Power_Task100ms(void);
bool APP_Power_IsHoldActive(void);
app_power_state_t APP_Power_GetState(void);
void APP_Power_EnterBootHoldLoop(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_APP_POWER_H_ */
