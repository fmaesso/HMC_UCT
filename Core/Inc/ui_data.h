#ifndef INC_UI_DATA_H_
#define INC_UI_DATA_H_

#include <stdint.h>
#include "main.h"
#include "cec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    char Ind_DataHora[10];
    uint8_t Ind_Bateria;
    uint8_t Ind_Energia;
} TDataBarraSuperior;

typedef struct{
    float Ind_Press1;
    float Ind_Press2;
    float Ind_PressMed;
    float Ind_Fluxo;
    int16_t RPM;
    int16_t RPMBack;
    int16_t MotorOK;
    uint8_t ErroRPM;
    uint8_t LowRPM;
    uint8_t Refresh;
    uint32_t Timer1;
    uint32_t Timer2;
    uint32_t Timer3;
    uint32_t Timer4;
    uint8_t TimerRun1;
    uint8_t TimerRun2;
    uint8_t TimerRun3;
    uint8_t TimerRun4;
} TTelaPrincipal;

void ui_getdataBarra(TDataBarraSuperior *out);
void ui_getdataPrincipal(TTelaPrincipal *out);
void getDataConfigTCX(configs_t *cfg_data);
void setDataConfigTCX(configs_t *cfg_data);
void getDataCECTCX(cec_info_t *cec_data);

void ResetaErroRPM(void);
void ResetaLowRPM(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_UI_DATA_H_ */
