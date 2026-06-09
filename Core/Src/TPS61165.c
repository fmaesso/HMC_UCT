/*
 * TPS61165.c
 *
 *  Created on: 5 de fev. de 2026
 *      Author: ferna
 */
#include "main.h"
#include "TPS61165.h"



#define ES_HIGH HAL_GPIO_WritePin(LCD_LED_LEVEL_GPIO_Port, LCD_LED_LEVEL_Pin, GPIO_PIN_SET)
#define ES_LOW HAL_GPIO_WritePin(LCD_LED_LEVEL_GPIO_Port, LCD_LED_LEVEL_Pin, GPIO_PIN_RESET)

//#define ES_LOW 		LCD_LED_LEVEL_GPIO_Port->ODR &= ~(1 << LCD_LED_LEVEL_Pin)
//#define ES_HIGH  	LCD_LED_LEVEL_GPIO_Port->ODR |= 1 << LCD_LED_LEVEL_Pin

void send_bit(uint8_t b);

static inline void delay_us(uint16_t us)
{
//    uint16_t start = __HAL_TIM_GET_COUNTER(&htim12);
//    while ((__HAL_TIM_GET_COUNTER(&htim12) - start) < us);
    __HAL_TIM_SET_COUNTER(&htim12,0);  // set the counter value a 0
    while (__HAL_TIM_GET_COUNTER(&htim12) < us);
}


void TPS61165_EasyScale_Enable(void) {
	// 1. FORCE SHUTDOWN
	    // If the system is already ON, we must reset the internal logic.
	    ES_LOW;
	    HAL_Delay(5); // Wait > 2.5ms to ensure shutdown
    // Mode Selection Handshake
    ES_HIGH;
    delay_us(100); // > tes_delay (100us)
    ES_LOW;
    delay_us(260); // > tes_det (260us)
    ES_HIGH;     // Device is now locked in EasyScale mode
    delay_us(50);
}

void TPS61165_EasyScale_Set(uint8_t level) {
    if (level > 31) level = 31;

    // 1. Device Address Byte (0x72)
    // 0111 0010 MSB first [cite: 861, 949]
    send_bit(0); send_bit(1); send_bit(1); send_bit(1);
    send_bit(0); send_bit(0); send_bit(1); send_bit(0);
    ES_HIGH; delay_us(5); // EOS [cite: 953]

	ES_LOW; delay_us(5); // EOS
    // 2. Start Condition for Data
    ES_HIGH; delay_us(5); // Ensure line is high before start

    // 3. DATABYTE: RFA=0, A1=0, A0=0, D4-D0=Level
    send_bit(0); // RFA
    send_bit(0); // A1
    send_bit(0); // A0
    for (int8_t i = 4; i >= 0; i--) {
        send_bit((level >> i) & 0x01); // D4 to D0
    }

	ES_LOW; delay_us(5); // EOS
    ES_HIGH; delay_us(30); // Final EOS to latch
}

void send_bit(uint8_t b)
{
    if (b == 0) {
    	ES_LOW;
        delay_us(15);   // 0-bit low
        ES_HIGH;
        delay_us(5);
    } else {
    	ES_LOW;
        delay_us(5);  // 1-bit low
        ES_HIGH;
        delay_us(15);
    }
}


