/*
 * cec.c
 *
 *  Created on: Dec 15, 2025
 *      Author: ferna
 */
#include "main.h"
#include "cec.h"
#include "utils.h"
#include "leds.h"
#include <string.h>
#include "ctype.h"
#include "stdio.h"

#include <stdio.h>
#include <stdlib.h>

//#include "quadspi.h"
#include "quadspi_IS.h"
#include "TPS61165.h"
#include "Tmp75.h"
//#include "display.h"
#include "encoder.h"
#include "ui_data.h"
#include "drv_gt911.h"
#include "arquivos.h"
#include "fault_log.h"
#include "press.h"
#include "app_power.h"
#include "app_watchdog.h"

#include "motor_soft_control.h"
#include "drv_proto_common.h"
#include "drv_proto_6step.h"
#include "drv_proto_foc.h"
#include "drv_motor_if.h"

uint8_t readbf[30];

extern volatile unsigned char BitTempo10ms, BitTempo100ms, BitTempo500ms, BitTempo1s, BitTempo2s, BitTempo5s, DivTimeLed, TempoPadraoLed;
extern TSerial Serial;
extern TLed Led[2];
extern uint8_t readbf[30];
extern uint8_t BufferFile[];
extern configs_t Configs;

extern TTimers Timers;
extern UART_HandleTypeDef huart4;
extern ADC_HandleTypeDef hadc1;
extern RTC_HandleTypeDef hrtc;

extern DRV_Common_t g_drv;
extern DRV_Callbacks_t drv_callbacks;

extern adc_type adc_buffer[2];
extern TPressao SensoresPressao[];
extern uint8_t VideoRefresh;
//MotorSoftControl_t g_msc;

extern MotorSoftControl_t g_msc;

TShowTimers ShowTimers;

EncoderEvent ev;
EncoderContext enc_ctx;

TDisplay DisplayStatus;

//extern uint8_t  gt911_pending;
extern uint8_t g_irqPending;

uint8_t MotorRun = 0;

#define RX_BUF_LEN	2
uint8_t RX_Buf_uart4[RX_BUF_LEN];


void printxy(void){
//	int32_t	x, y;
//	bool p;
//	if(DRV_GT911_ST_ReadTouch(&x, &y, &p)){
//		printf("X: %d - Y: %d - P: %d\r\n", x, y, p);
//	}
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef * hrtc){
	RunShowTimers();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart4)
    {
    	DRV_Common_OnRxBytes(&g_drv, RX_Buf_uart4, 1);
        HAL_UART_Receive_IT(&huart4, &RX_Buf_uart4[0], 1);
    }
}


void ExecTimers(void){
	if(Timers.TimerCliqueEncoderCpy){
		Timers.TimerCliqueEncoderCpy--;
		if(!Timers.TimerCliqueEncoderCpy){
			Timers.TimerCliqueEncoderBit = 1;
		}
	}
}



void RunShowTimers(void){
	for (int i = 0; i < 4; i++){
		if(ShowTimers.ShowTimerCountRun[i]){
			if(ShowTimers.ShowTimerType[i]){
				ShowTimers.ShowTimerCount[i]++;
				if(ShowTimers.ShowTimerCount[i] >= ShowTimers.ShowTimerAlarm[i]){
					ShowTimers.ShowTimerAlarmBit[i] = 1;
				}
			}else{
				if(ShowTimers.ShowTimerCount[i]){
					ShowTimers.ShowTimerCount[i]--;
					if(ShowTimers.ShowTimerCount[i]){
						ShowTimers.ShowTimerAlarmBit[i] = 1;
					}
				}
			}
		}
	}
}

void TimerTask(void){
	if(BitTempo10ms){
		APP_Watchdog_Mark(APP_WDG_HEARTBEAT_TIMERS);

		while (Encoder_GetEvent(&ev)) {
		    Encoder_ApplyEvent(&enc_ctx, &ev);
		}

	    if(MotorRun){
	    	DRV_Common_Task10ms(&g_drv);
	    	MSC_Task10ms(&g_msc);
	    }
		ExecLed();
		APP_Watchdog_Task10ms();
		BitTempo10ms = 0;
	}
	if(BitTempo100ms){
		APP_Power_Task100ms();

	    if(MotorRun){
	    	DRV_Common_Task100ms(&g_drv);
	    }
		ExecTimers();
		if(enc_ctx.Pre_SW){
			ExecEncSW();
		}
        HAL_ADC_Start_DMA(&hadc1,(uint32_t *) &adc_buffer, 2);
		BitTempo100ms = 0;
	}
	if(BitTempo500ms){
		VideoRefresh = 1;
		BitTempo500ms = 0;
	}
	if(BitTempo1s){
//		RunShowTimers();
		BitTempo1s = 0;
	}
	if(BitTempo2s){
		if(!MotorRun){
			InitMotor();
			MotorRun = 1;
		}
		BitTempo2s = 0;
	}
	if(BitTempo5s){
		BitTempo5s = 0;
	}
}

void TestExtFlash(uint32_t v){
	memory_dump((uint8_t *) 0x90800000, v);
	printf("\r\nDump 0x90800000\r\n");
}

void ReadFlash(void){
	CSP_QSPI_Read(readbf, 0, 20);
	memory_dump(readbf, 20);
}

void WriteFlash(uint8_t v){
	uint8_t wrb[20];
	for(int i =0; i < 20; i++){
		wrb[i] = v + i + 0x30;
	}
	CSP_QSPI_DisableMemoryMappedMode();
	CSP_QSPI_Erase_Block(0x800000);
	CSP_QSPI_Write(wrb, 0x800000, 20);
	CSP_QSPI_EnableMemoryMappedMode();
}

void ExecDisplay(char *S){
	GetDados((S + 1), 2);
	uint32_t v = atoi(StrValores[0]);
	switch(*S){
		case 'L':	TPS61165_EasyScale_Set(v);		//DL95;
					break;
	}
}

void ExecLedDisplayPlus(void){
	if(DisplayStatus.Led < 31){
		DisplayStatus.Led+= 2;
		TPS61165_EasyScale_Set(DisplayStatus.Led);
	}
}

void ExecLedDisplayMinus(void){
	if(DisplayStatus.Led > 2){
		DisplayStatus.Led-= 2;
		TPS61165_EasyScale_Set(DisplayStatus.Led);
	}
}


void InitTimer(void){
	memset((uint8_t *)&Timers, 0, sizeof(TTimers));
	Timers.TimerCliqueEncoder = TIME_TO_ENCODER;	//tempo de presso para gerar interrpcao do sw do encoder
	Timers.TimerCEC1 = Configs.timer[0].tempo;
	Timers.TimerCEC2 = Configs.timer[1].tempo;
	Timers.TimerCEC3 = Configs.timer[2].tempo;
	Timers.TimerCEC4 = Configs.timer[3].tempo;
}

void InitShowTimers(void){
	for(int i = 0; i < 4; i++){
		if(Configs.timer[i].sentido == 'u'){
			ShowTimers.ShowTimerType[i] = CRONOMETRO;
			ShowTimers.ShowTimerAlarm[i] = Configs.timer[i].tempo;
			ShowTimers.ShowTimerCount[i] = 0;
		}else{
			ShowTimers.ShowTimerType[i] = TEMPORIZADOR;
			ShowTimers.ShowTimerCount[i] = Configs.timer[i].tempo;
			ShowTimers.ShowTimerAlarm[i] = 0;
		}
	}
}


void InitHemocor(void){
//	uint8_t id[4];
	GetConfig();

	HAL_TIM_Base_Start(&htim12);  // start the Timer1
	InitLed();
	InitTimer();
	DisplayStatus.Led = 16;
	Encoder_Init();
	TPS61165_EasyScale_Enable();
	TPS61165_EasyScale_Set(16);
	DRV_GT911_ST_Init();
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    memset((uint8_t *)&SensoresPressao[0], 0, sizeof(TPressao));
    memset((uint8_t *)&SensoresPressao[1], 0, sizeof(TPressao));
    HAL_ADC_Start_DMA(&hadc1, adc_buffer, 2);
    InitShowTimers();
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2047, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
//    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 5, 0);
//    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

}

float LeTemp(void){
	float temp;
	One_ShotTemp();
	Read_TempCelsius(&temp);
	printf("temp: %f\r\n", temp);
	return temp;
}

void ExecFiles(char *S){
	GetDados((S + 1), 2);
	switch((char)*S){
		case 'L': listDir();								//FL
					break;

		case 'R': ReadFile(StrValores[0], BufferFile);		//FR\cec.json
					printf((char *)BufferFile);
					break;

		case 'W': WriteFile(StrValores[0], StrValores[1]);	//FW\teste2.txt;12345667890
					break;

		case 'D': DeleteFile(StrValores[0]);				//FD\cec.json
					break;

		case 'F': FormataMem();
					break;
	}
}


void ExecConfigs(uint8_t *S){
	bool b = false;
	GetDados((char *)(S + 1), 2);
	switch((char)*S){
		case 'F': 	SetFabrica();
					break;

		case 'D':	b = true;
		case 'd':	DRV_Common_SetDebugData(&g_drv, b);  //Cd
					break;

		case 'I':	InitMotor();
	    			MotorRun = 1;
	    			break;

		case 'S':   if(MotorRun){
						DRV_MotorIf_SendStart(&g_drv);		//CS
					}
					break;

		case 'P':   if(MotorRun){
						DRV_MotorIf_SendStop(&g_drv);		//CP
					}
					break;

		case 'L':   if(MotorRun){
						DRV_MotorIf_SendFaultAck(&g_drv);	//CF
					}
					break;

		case 'R':   if(MotorRun){
						DRV_MotorIf_SendSpeedRamp(&g_drv, atoi(StrValores[0]), atoi(StrValores[1]));	//CR2000;1000
					}
					break;

		case 'C':   MSC_ClearAggressiveLock(&g_msc);
					break;

	}
}

void ExecErroHard(uint8_t *S){
//	volatile uint32_t *p = (uint32_t*)0x00000001;
//	int a, b = 0;
	switch((char)*S){
		case 'C':	FaultLog_Clear();

		case 'S':   FaultLog_Print();
					break;
	}

}




void ExecCmd(uint8_t *S){
//	uint8_t S2[10];
//	GetDados((char *)S, 2);
	uint32_t v;
	switch((char)*S){
		case 'F':	ExecFiles((char *)(S + 1));
					break;

		case 'C':	ExecConfigs((S + 1));
					break;

		case 'E':	ExecErroHard((S + 1));
					break;

		case 'D':	ExecDisplay((char *)(S + 1));
					break;

//		case 'M': 	ReadFlash();
//					break;

		case 'm':	v = atol((char *)(S + 1));		//m200
					TestExtFlash(v);
					break;
//
		case 'W': 	v = atoi((char *)(S + 1));
					WriteFlash(v);
					break;

		case 'T':	LeTemp();
					break;

		case 'I': 	i2c_detect();
					break;

		case 'Z':	CSP_QSPI_Erase_Chip();
					break;

		case 'J':	CSP_QSPI_EnableMemoryMappedMode();
					break;

		case 'r': 	NVIC_SystemReset();
					break;
	}
	printf("OK\r\n");
	Led[LED_AZUL].QuantPisca = 7;
	Led[LED_AZUL].TimerOn = 2;
	Led[LED_AZUL].TimerOff = 2;
	Led[LED_AZUL].TempoOn = 2;
	Led[LED_AZUL].TempoOff = 2;
	memset((uint8_t *)&Serial.Cmd[0], 0, sizeof(Serial.Cmd));
}



void runCEC(void){
	if(Serial.Ok){
	  Serial.Ok = 0;
	  ExecCmd(Serial.Cmd);
	}

	TimerTask();
}




