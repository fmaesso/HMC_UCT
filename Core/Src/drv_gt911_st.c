/*
 * drv_gt911_st.c
 *
 *  Created on: 12 de fev. de 2026
 *      Author: ferna
 */




#include "drv_gt911.h"
#include "gt911.h"
#include "main.h"
#include "stm32h7xx_hal.h"

extern I2C_HandleTypeDef hi2c3;

#define GT911_ADDR_8BIT   (0x5D << 1)
//#define GT911_ADDR_8BIT   (0x14 << 1)

static GT911_Object_t g_gt;
uint8_t g_irqPending = 0;

void DRV_GT911_ST_NotifyIrq(void)
{
    g_irqPending = 1;
}

/* ---- IO callbacks ---- */
static int32_t IO_Init(void)   { return 0; }
static int32_t IO_DeInit(void) { return 0; }
static int32_t IO_GetTick(void){ return (int32_t)HAL_GetTick(); }

int32_t IO_ReadReg(uint16_t devAddr, uint16_t reg, uint8_t* data, uint16_t len)
{
    uint8_t a[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    if (HAL_I2C_Master_Transmit(&hi2c3, (uint16_t)devAddr, a, 2, 500) != HAL_OK) return -1;
    if (HAL_I2C_Master_Receive(&hi2c3, (uint16_t)devAddr, data, len, 500) != HAL_OK) return -1;
    return 0;
}

static int32_t IO_WriteReg(uint16_t devAddr, uint16_t reg, uint8_t* data, uint16_t len)
{
    uint8_t buf[2 + 256];
    if (len > 256) return -1;

    buf[0] = (uint8_t)(reg >> 8);
    buf[1] = (uint8_t)(reg & 0xFF);
    for (uint16_t i = 0; i < len; i++) buf[2+i] = data[i];

    if (HAL_I2C_Master_Transmit(&hi2c3, (uint16_t)devAddr, buf, (uint16_t)(2 + len), 50) != HAL_OK) return -1;
    return 0;
}

/* ---- Reset/addr select (0x5D) ---- */
//static void GT911_Reset_Select0x5D(void)
//{
//    /* INT HIGH durante reset para 0x5D (e depois vira entrada/EXTI) */
//    HAL_GPIO_WritePin(TS_INT_GPIO_Port, TS_INT_Pin, GPIO_PIN_SET);
//
//    HAL_GPIO_WritePin(TS_RST_GPIO_Port, TS_RST_Pin, GPIO_PIN_RESET);
//    HAL_Delay(10);
//    HAL_GPIO_WritePin(TS_RST_GPIO_Port, TS_RST_Pin, GPIO_PIN_SET);
//    HAL_Delay(60);
//}

#include "main.h"
#include "stm32h7xx_hal.h"

static void GT911_SelectAddr_0x5D_Reset(void)
{
    GPIO_InitTypeDef gi = {0};

    // INT como saída (push-pull), forçando LOW => seleciona 0x5D (como no Arduino)
    gi.Pin = TS_INT_Pin;
    gi.Mode = GPIO_MODE_OUTPUT_PP;
    gi.Pull = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TS_INT_GPIO_Port, &gi);
    HAL_GPIO_WritePin(TS_INT_GPIO_Port, TS_INT_Pin, GPIO_PIN_RESET);

    // RST como saída e LOW (assert reset)
    gi.Pin = TS_RST_Pin;
    gi.Mode = GPIO_MODE_OUTPUT_PP;
    gi.Pull = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TS_RST_GPIO_Port, &gi);
    HAL_GPIO_WritePin(TS_RST_GPIO_Port, TS_RST_Pin, GPIO_PIN_RESET);

    // T2 > 10ms
    HAL_Delay(11);

    // T3 > 100us
    // solta o reset colocando o pino em alta impedância (entrada)
    gi.Pin = TS_RST_Pin;
    gi.Mode = GPIO_MODE_INPUT;      // “solta” o reset
    gi.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(TS_RST_GPIO_Port, &gi);
    // aguarda
    for (volatile int i=0; i<3000; i++) { __NOP(); }  // ~>100us (depende do clock)

    // T4 > 5ms
    HAL_Delay(6);

    // mantém INT LOW mais um pouco (Arduino faz "pinHold" e depois 50ms+50ms)
    HAL_Delay(51);

    // depois libera INT para virar entrada/EXTI (Arduino deixa input flutuante)
    gi.Pin = TS_INT_Pin;
    gi.Mode = GPIO_MODE_IT_RISING;   // Arduino usa RISING
    gi.Pull = GPIO_NOPULL;           // Arduino comenta “no pullups”
    HAL_GPIO_Init(TS_INT_GPIO_Port, &gi);

    HAL_Delay(51);
}




bool DRV_GT911_ST_Init(void)
{
    GT911_IO_t io = {0};
    io.Init     = IO_Init;
    io.DeInit   = IO_DeInit;
    io.Address  = GT911_ADDR_8BIT;
    io.ReadReg  = IO_ReadReg;
    io.WriteReg = IO_WriteReg;
    io.GetTick  = IO_GetTick;

//    GT911_Reset_Select0x5D();
    GT911_SelectAddr_0x5D_Reset();

//    int ok5D = (HAL_I2C_IsDeviceReady(&hi2c3, 0x5D<<1, 2, 20) == HAL_OK);
//    int ok14 = (HAL_I2C_IsDeviceReady(&hi2c3, 0x14<<1, 2, 20) == HAL_OK);


    if (GT911_RegisterBusIO(&g_gt, &io) != GT911_OK) return false;

    /* Opcional: setar modo de trigger no chip (rising/falling) */
    GT911_SetTriggerMode(&g_gt, GT911_M_SW1_INTERRUPT_FALLING);   // ou RISING
    if (GT911_EnableIT(&g_gt) != GT911_OK) return false;

    if (GT911_Init(&g_gt) != GT911_OK) return false;

    g_irqPending = 0;
    return true;
}

bool DRV_GT911_ST_ReadTouch(int32_t* x, int32_t* y, bool* pressed)
{
	{
	    if (!x || !y || !pressed) return false;

	    /* Usa interrupção como “gate” (economia) */
	    if (!g_irqPending) { *pressed = false; return true; }
	    g_irqPending = 0;

	    GT911_State_t st = {0};

	    /* Melhorar bug da ST: DetectTouch é chamado 2x dentro de GetState.
	       Alternativa simples: usar MultiTouchState (também limpa depois) ou
	       patchar gt911.c. Por enquanto, use GetState e aceite custo. */
	    if (GT911_GetState(&g_gt, &st) != GT911_OK) {
	        *pressed = false;
	        return false;
	    }

	    if (st.TouchDetected == 0) {
	        *pressed = false;
	        return true;
	    }

	    *pressed = true;
	    *x = (int32_t)st.TouchX;
	    *y = (int32_t)st.TouchY;
//	    printf("X: %d - Y: %d\r\n", st.TouchX, st.TouchY);
	    return true;
	}
}

