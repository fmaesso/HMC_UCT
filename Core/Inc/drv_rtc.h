/*
 * drv_rtc.h
 *
 *  Created on: 11 de fev. de 2026
 *      Author: ferna
 */
#pragma once
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t year;   // 2000..2099
    uint8_t  month;  // 1..12
    uint8_t  day;    // 1..31
    uint8_t  hour;   // 0..23
    uint8_t  min;    // 0..59
    uint8_t  sec;    // 0..59
    uint8_t  wday;   // 1..7 (HAL: Monday=1 ... Sunday=7)
} rtc_datetime_t;

/* Inicializa o RTC (não reseta data/hora se já estiver inicializado) */
bool DRV_RTC_InitSafe(void);

/* Força reinicialização do RTC e grava uma data/hora (use com cuidado) */
bool DRV_RTC_ForceInitAndSet(const rtc_datetime_t* dt);

/* Set/Get (formato BIN) */
bool DRV_RTC_SetDateTime(const rtc_datetime_t* dt);
bool DRV_RTC_GetDateTime(rtc_datetime_t* dt);

/* Helpers */
uint32_t DRV_RTC_ToUnix(const rtc_datetime_t* dt);           // opcional (2000..2099)
bool     DRV_RTC_FromUnix(uint32_t unix_s, rtc_datetime_t* dt);

void GetStrTime(char *StrTime);

#ifdef __cplusplus
}
#endif




