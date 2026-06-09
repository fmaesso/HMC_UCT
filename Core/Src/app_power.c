#include "app_power.h"
#include "app_watchdog.h"
#include "board_uct.h"
#include "TPS61165.h"
#include "main.h"

static app_power_state_t g_power_state = APP_POWER_STATE_BOOT_CHECK_HOLD;

void APP_Power_Init(void)
{
    BOARD_UCT_InitSafetyPins();

    if (BOARD_UCT_IsHoldActive()) {
        g_power_state = APP_POWER_STATE_HOLD_LOOP;
        BOARD_UCT_SetMotorHold(true);
    } else {
        g_power_state = APP_POWER_STATE_NORMAL;
        BOARD_UCT_SetMotorHold(false);
    }
}

void APP_Power_Task100ms(void)
{
    if (BOARD_UCT_IsHoldActive()) {
        g_power_state = APP_POWER_STATE_HOLD_LOOP;
        BOARD_UCT_SetMotorHold(true);
    } else if (g_power_state == APP_POWER_STATE_HOLD_LOOP) {
        g_power_state = APP_POWER_STATE_NORMAL;
        BOARD_UCT_SetMotorHold(false);
    }
}

bool APP_Power_IsHoldActive(void)
{
    return BOARD_UCT_IsHoldActive();
}

app_power_state_t APP_Power_GetState(void)
{
    return g_power_state;
}

void APP_Power_EnterBootHoldLoop(void)
{
    g_power_state = APP_POWER_STATE_HOLD_LOOP;
    BOARD_UCT_SetMotorHold(true);
    HAL_TIM_Base_Start(&htim12);
    TPS61165_EasyScale_Enable();
    TPS61165_EasyScale_Set(0);

    while (BOARD_UCT_IsHoldActive()) {
        BOARD_UCT_MirrorMotorHold();
        APP_Watchdog_ServiceHoldLoop();
        HAL_Delay(10);
    }

    NVIC_SystemReset();
}
