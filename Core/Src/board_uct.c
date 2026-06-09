#include "board_uct.h"

void BOARD_UCT_InitSafetyPins(void)
{
    BOARD_UCT_SetExternalWatchdog(false);
    BOARD_UCT_MirrorMotorHold();
}

bool BOARD_UCT_IsHoldActive(void)
{
    return (HAL_GPIO_ReadPin(HOLD_PIN_GPIO_Port, HOLD_PIN_Pin) == GPIO_PIN_SET);
}

void BOARD_UCT_SetMotorHold(bool active)
{
    HAL_GPIO_WritePin(MOT_DRV_HOLD_GPIO_Port,
                      MOT_DRV_HOLD_Pin,
                      active ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void BOARD_UCT_MirrorMotorHold(void)
{
    BOARD_UCT_SetMotorHold(BOARD_UCT_IsHoldActive());
}

void BOARD_UCT_SetExternalWatchdog(bool high)
{
    HAL_GPIO_WritePin(WDI_GPIO_Port, WDI_Pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void BOARD_UCT_ToggleExternalWatchdog(void)
{
    HAL_GPIO_TogglePin(WDI_GPIO_Port, WDI_Pin);
}
