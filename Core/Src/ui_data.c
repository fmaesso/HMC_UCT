/*
 * ui_data.c
 *
 *  Created on: 11 de fev. de 2026
 *      Author: ferna
 */

#include "main.h"
#include "ui_data.h"
#include "encoder.h"
#include "drv_rtc.h"
#include "cec.h"
#include "arquivos.h"
#include <string.h>
#include "drv_proto_common.h"
#include "motor_soft_control.h"
#include "app_ui_model.h"

extern configs_t Configs;
extern cec_info_t cec;

extern EncoderContext enc_ctx;
extern DRV_Common_t g_drv;
extern MotorSoftControl_t g_msc;
extern TPressao SensoresPressao[];
//extern uint8_t VideoRefresh;
extern TShowTimers ShowTimers;


//TDataBarraSuperior DataBarraSuperior;
//TTelaPrincipal TelaPrincipal;

void ui_getdataBarra(TDataBarraSuperior *out){
	APP_UI_Model_GetBarra(out);
}

void ui_getdataPrincipal(TTelaPrincipal *out){
	APP_UI_Model_GetPrincipal(out);
}

void getDataConfigTCX(configs_t *cfg_data){
	memcpy((uint8_t *)cfg_data, (uint8_t *)&Configs, sizeof(configs_t));
}

void setDataConfigTCX(configs_t *cfg_data){
	memcpy((uint8_t *)&Configs, (uint8_t *)cfg_data, sizeof(configs_t));
	SetConfig();
	GetConfig();
}


void getDataCECTCX(cec_info_t *cec_data){
	memcpy((uint8_t *)cec_data, (uint8_t *)&cec, sizeof(cec_info_t));
}


void ResetaErroRPM(void){
	MSC_ClearAggressiveLock(&g_msc);
}

void ResetaLowRPM(void){
	MSC_ClearLowerLimitLock(&g_msc);
}
