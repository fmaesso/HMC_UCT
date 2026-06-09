/*
 * leds.c
 *
 *  Created on: Jan 2, 2025
 *      Author: fmaes
 */

#include "main.h"
#include "utils.h"
#include "cec.h"
#include "leds.h"
#include <stdbool.h>

extern TLed Led[];

void InitLed(void){
	Led[LED_AZUL].LedPort = LED1_GPIO_Port;
	Led[LED_AZUL].LedPin = LED1_Pin;
	Led[LED_AZUL].Tipo = NO_LED;
	Led[LED_AZUL].Status = false;
	Led[LED_AZUL].TimerOn = 500 / 10;
	Led[LED_AZUL].TimerOff = 500	/ 10;
	Led[LED_AZUL].TempoOn = 500 / 10;
	Led[LED_AZUL].TempoOff = 500	/ 10;
	Led[LED_AZUL].QuantPisca = 0;

	Led[LED_VERMELHO].LedPort = LED2_GPIO_Port;
	Led[LED_VERMELHO].LedPin = LED2_Pin;
	Led[LED_VERMELHO].Tipo = LED_PISCA;
	Led[LED_VERMELHO].TimerOn = 500	/ 10;
	Led[LED_VERMELHO].TimerOff = 500 / 10;
	Led[LED_VERMELHO].TempoOn = 500	/ 10;
	Led[LED_VERMELHO].TempoOff = 500 / 10;
	Led[LED_VERMELHO].Status = false;
	Led[LED_VERMELHO].QuantPisca = 0;

	LedOff(LED_AZUL);
	LedOn(LED_VERMELHO);
}

void LedOn(int l){
//	HAL_GPIO_WritePin(Led[l].LedPort, Led[l].LedPin, GPIO_PIN_RESET);
	Led[l].LedPort->ODR &= ~Led[l].LedPin;
	Led[l].Status = true;

}

void LedOff(int l){
//	HAL_GPIO_WritePin(Led[l].LedPort, Led[l].LedPin, GPIO_PIN_SET);
	Led[l].LedPort->ODR |= Led[l].LedPin;
	Led[l].Status = false;
}


void LedChange(int l){
	if(Led[l].Status){
		if(!Led[l].TimerOn--){
			Led[l].TimerOn = Led[l].TempoOn;
			LedOff(l);
			if(Led[l].QuantPisca){
				Led[l].QuantPisca--;
			}
		}
	}else{
		if(!Led[l].TimerOff--){
			Led[l].TimerOff = Led[l].TempoOff;
			LedOn(l);
			if(Led[l].QuantPisca){
				Led[l].QuantPisca--;
			}
		}
	}
}



//***********************************************************
//
//***********************************************************
void Complementa_Led(int l){

	if(Led[l].Status){
		LedOff(l);
	}else{
		LedOn(l);
	}
	Led[l].Tipo = NO_LED;
}



void ExecLed(void){
	for(int l = 0; l < 2; l++){
		switch(Led[l].Tipo){
			case LED_PISCA: LedChange(l);
							break;

			case LED_COMPL:	Complementa_Led(l);
							break;
		}
		if(Led[l].QuantPisca){
			LedChange(l);
		}
	}
}





