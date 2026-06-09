#ifndef INC_APP_WATCHDOG_H_
#define INC_APP_WATCHDOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    APP_WDG_HEARTBEAT_LOOP = 0,
    APP_WDG_HEARTBEAT_TIMERS,
    APP_WDG_HEARTBEAT_UI,
    APP_WDG_HEARTBEAT_COUNT
} app_wdg_heartbeat_t;

void APP_Watchdog_Init(void);
void APP_Watchdog_Mark(app_wdg_heartbeat_t heartbeat);
void APP_Watchdog_SetCriticalFault(bool active);
bool APP_Watchdog_IsHealthy(void);
void APP_Watchdog_Task10ms(void);
void APP_Watchdog_ServiceHoldLoop(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_APP_WATCHDOG_H_ */
