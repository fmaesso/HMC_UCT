/*
 * fault_log.h
 *
 *  Created on: 21 de jan. de 2026
 *      Author: ferna
 */

#ifndef INC_FAULT_LOG_H_
#define INC_FAULT_LOG_H_

#pragma once
#include <stdint.h>

typedef struct
{
  uint32_t r0, r1, r2, r3, r12, lr, pc, psr;
} fault_stack_t;

void FaultLog_SaveFromFrame(const fault_stack_t *frame,
                            uint32_t cfsr, uint32_t hfsr, uint32_t dfsr,
                            uint32_t afsr, uint32_t bfar, uint32_t mmfar,
                            uint32_t icsr,
                            uint32_t uptime_ms);

void FaultLog_Print(void);
void FaultLog_Clear(void);

/* Decode “humano” */
void Fault_PrintDecoded(uint32_t cfsr, uint32_t hfsr, uint32_t bfar, uint32_t mmfar);


#endif /* INC_FAULT_LOG_H_ */
