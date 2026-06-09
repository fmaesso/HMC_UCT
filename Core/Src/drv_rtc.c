#include "main.h"
#include "drv_rtc.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdio.h>

/* hrtc deve ser gerado pelo CubeMX (rtc.c) */
extern RTC_HandleTypeDef hrtc;

/* Backup register “magic” para saber se RTC já foi inicializado */
#define RTC_BKP_MAGIC      0x32F2u
#define RTC_BKP_REG_MAGIC  RTC_BKP_DR0

/* Converte Year 2000..2099 para HAL Year 0..99 */
static uint8_t year_to_hal(uint16_t year)
{
    if (year < 2000) year = 2000;
    if (year > 2099) year = 2099;
    return (uint8_t)(year - 2000);
}

static uint16_t year_from_hal(uint8_t y)
{
    return (uint16_t)(2000u + (uint16_t)y);
}

static bool rtc_set_bin(const rtc_datetime_t* dt)
{
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    /* Hora em 24h */
    t.TimeFormat = RTC_HOURFORMAT_24;
    t.Hours      = dt->hour;
    t.Minutes    = dt->min;
    t.Seconds    = dt->sec;
    t.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    t.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &t, RTC_FORMAT_BIN) != HAL_OK)
        return false;

    d.Year    = year_to_hal(dt->year);
    d.Month   = dt->month;
    d.Date    = dt->day;
    d.WeekDay = (dt->wday >= 1 && dt->wday <= 7) ? dt->wday : RTC_WEEKDAY_MONDAY;

    if (HAL_RTC_SetDate(&hrtc, &d, RTC_FORMAT_BIN) != HAL_OK)
        return false;

    return true;
}

bool DRV_RTC_SetDateTime(const rtc_datetime_t* dt)
{
    if (!dt) return false;
    return rtc_set_bin(dt);
}

bool DRV_RTC_GetDateTime(rtc_datetime_t* dt)
{
    if (!dt) return false;

    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    /* Importante: GetTime e em seguida GetDate (ordem da HAL) */
    if (HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN) != HAL_OK)
        return false;

    if (HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN) != HAL_OK)
        return false;

    dt->year  = year_from_hal(d.Year);
    dt->month = d.Month;
    dt->day   = d.Date;
    dt->wday  = d.WeekDay;

    dt->hour  = t.Hours;
    dt->min   = t.Minutes;
    dt->sec   = t.Seconds;

    return true;
}

bool DRV_RTC_InitSafe(void)
{
    /* Se o CubeMX já inicializou o RTC, aqui apenas checamos “magic” */
    uint32_t magic = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_REG_MAGIC);

    if (magic == RTC_BKP_MAGIC) {
        /* RTC já foi configurado antes */
        return true;
    }

    /* Primeiro boot (ou backup domain perdido): defina um default */
    rtc_datetime_t def = {
        .year = 2026, .month = 1, .day = 1,
        .hour = 0, .min = 0, .sec = 0,
        .wday = RTC_WEEKDAY_THURSDAY
    };

    if (!rtc_set_bin(&def))
        return false;

    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_REG_MAGIC, RTC_BKP_MAGIC);
    return true;
}

bool DRV_RTC_ForceInitAndSet(const rtc_datetime_t* dt)
{
    if (!dt) return false;

    /* Força o “não inicializado” e seta novamente */
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_REG_MAGIC, 0u);

    if (!rtc_set_bin(dt))
        return false;

    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_REG_MAGIC, RTC_BKP_MAGIC);
    return true;
}

/* ====== Helpers Unix (opcional) ======
   Implementação simples válida 2000..2099.
   Se não precisar, pode remover essas funções.
*/

static int is_leap(int y)
{
    return ((y % 4) == 0); /* válido para 2000..2099 */
}

static const uint16_t mdays_norm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

static uint32_t days_before_year(int y) /* dias desde 2000-01-01 até y-01-01 */
{
    uint32_t days = 0;
    for (int yy = 2000; yy < y; yy++)
        days += is_leap(yy) ? 366u : 365u;
    return days;
}

static uint32_t days_before_month(int y, int m) /* m:1..12, dias até m-01 */
{
    uint32_t days = 0;
    for (int mm = 1; mm < m; mm++) {
        uint16_t d = mdays_norm[mm-1];
        if (mm == 2 && is_leap(y)) d = 29;
        days += d;
    }
    return days;
}

/* Unix epoch 1970-01-01 -> aqui usamos base 2000-01-01 e somamos offset */
#define UNIX_OFFSET_2000 946684800u /* segundos entre 1970-01-01 e 2000-01-01 */

uint32_t DRV_RTC_ToUnix(const rtc_datetime_t* dt)
{
    if (!dt) return 0;

    int y = (int)dt->year;
    int m = (int)dt->month;
    int d = (int)dt->day;

    if (y < 2000) y = 2000;
    if (y > 2099) y = 2099;
    if (m < 1) m = 1;
    if (m > 12) m = 12;
    if (d < 1) d = 1;
    if (d > 31) d = 31;

    uint32_t days = days_before_year(y) + days_before_month(y, m) + (uint32_t)(d - 1);
    uint32_t sec  = (uint32_t)dt->hour * 3600u + (uint32_t)dt->min * 60u + (uint32_t)dt->sec;

    return UNIX_OFFSET_2000 + days * 86400u + sec;
}

bool DRV_RTC_FromUnix(uint32_t unix_s, rtc_datetime_t* dt)
{
    if (!dt) return false;
    if (unix_s < UNIX_OFFSET_2000) return false;

    uint32_t s = unix_s - UNIX_OFFSET_2000;
    uint32_t days = s / 86400u;
    uint32_t rem  = s % 86400u;

    dt->hour = (uint8_t)(rem / 3600u); rem %= 3600u;
    dt->min  = (uint8_t)(rem / 60u);
    dt->sec  = (uint8_t)(rem % 60u);

    int y = 2000;
    while (1) {
        uint32_t yd = is_leap(y) ? 366u : 365u;
        if (days < yd) break;
        days -= yd;
        y++;
        if (y > 2099) { y = 2099; break; }
    }
    dt->year = (uint16_t)y;

    int m = 1;
    while (m <= 12) {
        uint16_t md = mdays_norm[m-1];
        if (m == 2 && is_leap(y)) md = 29;
        if (days < md) break;
        days -= md;
        m++;
    }
    dt->month = (uint8_t)m;
    dt->day   = (uint8_t)(days + 1);

    /* weekday opcional: não calculo aqui; se precisar eu adiciono */
    dt->wday = RTC_WEEKDAY_MONDAY;
    return true;
}

void GetStrTime(char *StrTime)
{
    if (!StrTime) return;

    rtc_datetime_t now;

    if (!DRV_RTC_GetDateTime(&now)) {
        /* Em caso de erro, retorna string segura */
        sprintf(StrTime, "--:--:--");
        return;
    }

    /* Formato 24h: HH:MM:SS */
    sprintf(StrTime, "%02d:%02d:%02d",
            now.hour,
            now.min,
            now.sec);
}
