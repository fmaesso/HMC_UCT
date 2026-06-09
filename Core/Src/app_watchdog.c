#include "app_watchdog.h"
#include "board_uct.h"
#include "main.h"

static uint32_t g_last_mark[APP_WDG_HEARTBEAT_COUNT];
static bool g_critical_fault;

static const uint32_t g_timeout_ms[APP_WDG_HEARTBEAT_COUNT] = {
    250u,   /* main loop */
    500u,   /* periodic timers */
    1000u   /* TouchGFX tick/process */
};

void APP_Watchdog_Init(void)
{
    uint32_t now = HAL_GetTick();

    for (uint32_t i = 0u; i < APP_WDG_HEARTBEAT_COUNT; i++) {
        g_last_mark[i] = now;
    }
    g_critical_fault = false;
    BOARD_UCT_SetExternalWatchdog(false);
}

void APP_Watchdog_Mark(app_wdg_heartbeat_t heartbeat)
{
    if (heartbeat < APP_WDG_HEARTBEAT_COUNT) {
        g_last_mark[heartbeat] = HAL_GetTick();
    }
}

void APP_Watchdog_SetCriticalFault(bool active)
{
    g_critical_fault = active;
}

bool APP_Watchdog_IsHealthy(void)
{
    uint32_t now = HAL_GetTick();

    if (g_critical_fault) {
        return false;
    }

    for (uint32_t i = 0u; i < APP_WDG_HEARTBEAT_COUNT; i++) {
        if ((now - g_last_mark[i]) > g_timeout_ms[i]) {
            return false;
        }
    }

    return true;
}

void APP_Watchdog_Task10ms(void)
{
    if (APP_Watchdog_IsHealthy()) {
        BOARD_UCT_ToggleExternalWatchdog();
    }
}

void APP_Watchdog_ServiceHoldLoop(void)
{
    BOARD_UCT_ToggleExternalWatchdog();
}
