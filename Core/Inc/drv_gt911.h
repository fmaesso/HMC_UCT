/*
 * drv_gt911.h
 *
 *  Created on: 12 de fev. de 2026
 *      Author: ferna
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "gt911.h"

#ifdef __cplusplus
extern "C" {
#endif

bool DRV_GT911_ST_Init(void);
bool DRV_GT911_ST_ReadTouch(int32_t* x, int32_t* y, bool* pressed);

//bool DRV_GT911_ST_ReadTouch(GT911_State_t *st);
/* Chamar no EXTI callback do TS_INT */
void DRV_GT911_ST_NotifyIrq(void);

int32_t IO_ReadReg(uint16_t devAddr, uint16_t reg, uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif
