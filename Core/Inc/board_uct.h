#ifndef INC_BOARD_UCT_H_
#define INC_BOARD_UCT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

void BOARD_UCT_InitSafetyPins(void);
bool BOARD_UCT_IsHoldActive(void);
void BOARD_UCT_SetMotorHold(bool active);
void BOARD_UCT_MirrorMotorHold(void);
void BOARD_UCT_SetExternalWatchdog(bool high);
void BOARD_UCT_ToggleExternalWatchdog(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_BOARD_UCT_H_ */
