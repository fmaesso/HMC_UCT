#pragma once
/*
 * cec_json.h
 *
 * Estruturas e funções de GET/SET e parse/serialize para cec.json usando cJSON.
 *
 * Baseado no formato observado em cec.json (versao_fw, versao_hw, data_ativacao, modelo,
 * cec_sn, pws_sn, drv_sn, url, id_cliente, cliente, bateria).
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cec.h"

/* --------- Inicialização / validação --------- */
void CEC_InitDefaults(cec_info_t* cfg);
bool CEC_Validate(const cec_info_t* cfg);

/* --------- Parse / Serialize ---------
 * Observação: CEC_ToJsonString retorna um ponteiro alocado via cJSON.
 * Libere com CEC_FreeJsonString().
 */
bool  CEC_FromJsonString(const char* json, cec_info_t* out_cfg);
char* CEC_ToJsonString(const cec_info_t* cfg);
void  CEC_FreeJsonString(char* json_str);

/* --------- GETs --------- */
const char* CEC_GetVersaoFW(const cec_info_t* cfg);
const char* CEC_GetVersaoHW(const cec_info_t* cfg);
const char* CEC_GetDataAtivacao(const cec_info_t* cfg);
const char* CEC_GetModelo(const cec_info_t* cfg);
const char* CEC_GetCecSN(const cec_info_t* cfg);
const char* CEC_GetPwsSN(const cec_info_t* cfg);
const char* CEC_GetDrvSN(const cec_info_t* cfg);
const char* CEC_GetUrl(const cec_info_t* cfg);
const char* CEC_GetIdCliente(const cec_info_t* cfg);
const char* CEC_GetCliente(const cec_info_t* cfg);
const char* CEC_GetBateria(const cec_info_t* cfg);

/* --------- SETs (retornam false se truncar ou receber NULL) --------- */
bool CEC_SetVersaoFW(cec_info_t* cfg, const char* s);
bool CEC_SetVersaoHW(cec_info_t* cfg, const char* s);
bool CEC_SetDataAtivacao(cec_info_t* cfg, const char* s);
bool CEC_SetModelo(cec_info_t* cfg, const char* s);
bool CEC_SetCecSN(cec_info_t* cfg, const char* s);
bool CEC_SetPwsSN(cec_info_t* cfg, const char* s);
bool CEC_SetDrvSN(cec_info_t* cfg, const char* s);
bool CEC_SetUrl(cec_info_t* cfg, const char* s);
bool CEC_SetIdCliente(cec_info_t* cfg, const char* s);
bool CEC_SetCliente(cec_info_t* cfg, const char* s);
bool CEC_SetBateria(cec_info_t* cfg, const char* s);

#ifdef __cplusplus
}
#endif
