/*
 * cec.h
 *
 *  Created on: Dec 15, 2025
 *      Author: ferna
 */

#ifndef INC_CEC_H_
#define INC_CEC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

//Definicao dos controle dos led
#define LED_AZUL		0		//Led Azul
#define LED_VERMELHO	1		//Led Vermelho

#define LED_OFF		0
#define LED_ON		1
#define LED_PISCA	2
#define LED_COMPL	3
#define NO_LED		4

typedef struct {
    float Amin;
    float Amax;
    float Bmin;
    float Bmax;
} cfg_pressao_t;

typedef struct {
	float min;
	float max;
} cfg_range_t;

typedef struct {
    uint32_t tempo;     // ms
    char sentido;       // 'u' / 'd' / etc (pega 1o char do JSON)
    int  alarme;        // 0/1
} cfg_timer_t;

typedef struct {
    cfg_pressao_t pressao;
    cfg_range_t   fluxo;
    cfg_range_t   rpm;
    cfg_timer_t   timer[4];  // timer[0]=timer1 ... timer[3]=timer4
} configs_t;

#define CRONOMETRO		1
#define TEMPORIZADOR	0


typedef struct{
	uint32_t ShowTimerCount[4];
	uint32_t ShowTimerAlarm[4];
	uint32_t ShowTimerAlarmBit[4];
	uint8_t ShowTimerCountRun[4];
	uint8_t ShowTimerType[4];
}TShowTimers;


typedef struct
{
    char versao_fw[16];
    char versao_hw[16];
    char data_ativacao[16];   // ex: "20/02/2026"
    char modelo[32];

    char cec_sn[20];
    char pws_sn[20];
    char drv_sn[20];

    char url[96];

    char id_cliente[16];
    char cliente[32];

    char bateria[16];         // string de data (como está no JSON)
} cec_info_t;

#define PRESS1		0
#define PRESS2		1

#define adc_type	uint32_t


//typedef struct {
//	adc_type valMedia[50];
//	uint16_t Index;
//	uint64_t Soma;
//	float Pressao;
//	float V_Zero_ADC;
//	adc_type ultimoValorValido;
//	uint8_t Status;
//}TPressao;

typedef struct {
    float PressaoFiltrada; // O valor suavizado em mmHg ou PSI
    float Alfa;            // Fator de suavização (0.0 a 1.0)
    float V_Zero_ADC;
    float Pressao;         // Valor final para o display
    uint8_t Status;
} TPressao;


typedef struct{
	bool Status;
	uint32_t TimerOn;
	uint32_t TimerOff;
	uint32_t TempoOn;
	uint32_t TempoOff;
    GPIO_TypeDef *LedPort; // Porta GPIO (e.g., GPIOA, GPIOB)
    uint16_t LedPin;       // Pino GPIO (e.g., GPIO_PIN_0, GPIO_PIN_1)
	char Tipo;
	int QuantPisca;
}TLed;

typedef struct{
	volatile uint32_t TimerCliqueEncoder;
	volatile uint32_t TimerCliqueEncoderCpy;
	uint8_t TimerCliqueEncoderBit;

	volatile uint32_t TimerCEC1;
	volatile uint32_t TimerCEC1Cpy;
	uint8_t TimerCEC1Bit;

	volatile uint32_t TimerCEC2;
	volatile uint32_t TimerCEC2Cpy;
	uint8_t TimerCEC2Bit;

	volatile uint32_t TimerCEC3;
	volatile uint32_t TimerCEC3Cpy;
	uint8_t TimerCEC3Bit;

	volatile uint32_t TimerCEC4;
	volatile uint32_t TimerCEC4Cpy;
	uint8_t TimerCEC4Bit;
}TTimers;

//tempos para os timers em 100 milisegundos

#define TIME_TO_ENCODER		40	// 4 s em tarefas de 100 ms

enum AlarmeTipo{
	tpTimer = 1,
	tpPressao,
	tpFluxo,
	tpRPM
};

enum NivelAlarme{
	alScrLowLevel = 1,
	alScrMidLevel,
	alScrHigLevel,
	alSndLowLevel,
	alSndMidLevel,
	alSndHigLevel
};

enum OrigemAlarme{
	orPressaoAMin = 1,
	orPressaoAMax,
	orPressaoBMin,
	orPressaoBMax,
	orFluxoMin,
	orFluxoMax,
	orRPMMin,
	orRPMMax,
	orTimer1,
	orTimer2,
	orTimer3,
	orTimer4,
	NUM_OR_AL
};


typedef struct{
	uint8_t Origem;
	uint8_t Nivel;
}TAlarmes;


typedef struct{
	uint8_t *pSend;
	int Quant;
	uint8_t Cmd[256];
	uint8_t Ok;
	uint8_t *Ptr;
	uint8_t TXOk;
	uint8_t IsConected;
}TSerial;

typedef struct{
	uint8_t Tela;
	uint8_t Led;
}TDisplay;

void TimerTask(void);
void runCEC(void);
void ExecCmd(uint8_t *S);
void TestExtRam(void);
void TestExtFlash(uint32_t v);
void InitHemocor(void);
void RunShowTimers(void);

void ExecLedDisplayPlus(void);
void ExecLedDisplayMinus(void);
#ifdef __cplusplus
}
#endif
#endif /* INC_CEC_H_ */
