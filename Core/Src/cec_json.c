/*
 * cec_json.c
 *
 * Implementação para cec_json.h usando cJSON.
 */

#include "cec_json.h"
#include "cJSON.h"

#include <string.h>

/* --- helpers --- */
static bool copy_str(char* dst, size_t dst_sz, const char* src)
{
    if (!dst || dst_sz == 0 || !src) return false;
    size_t n = strlen(src);
    if (n >= dst_sz) {
        /* copia truncando, mas sinaliza false */
        memcpy(dst, src, dst_sz - 1);
        dst[dst_sz - 1] = '\0';
        return false;
    }
    memcpy(dst, src, n + 1);
    return true;
}

static const char* get_string_or_empty(const cJSON* obj, const char* key)
{
    const cJSON* it = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, key);
    if (cJSON_IsString(it) && it->valuestring) return it->valuestring;
    return "";
}

void CEC_InitDefaults(cec_info_t* cfg)
{
    if (!cfg) return;
    memset(cfg, 0, sizeof(*cfg));
    /* defaults seguros */
    copy_str(cfg->versao_fw, sizeof(cfg->versao_fw), "0.0.0");
    copy_str(cfg->versao_hw, sizeof(cfg->versao_hw), "0.0.0");
    copy_str(cfg->data_ativacao, sizeof(cfg->data_ativacao), "");
    copy_str(cfg->modelo, sizeof(cfg->modelo), "");
    copy_str(cfg->cec_sn, sizeof(cfg->cec_sn), "");
    copy_str(cfg->pws_sn, sizeof(cfg->pws_sn), "");
    copy_str(cfg->drv_sn, sizeof(cfg->drv_sn), "");
    copy_str(cfg->url, sizeof(cfg->url), "");
    copy_str(cfg->id_cliente, sizeof(cfg->id_cliente), "");
    copy_str(cfg->cliente, sizeof(cfg->cliente), "");
    copy_str(cfg->bateria, sizeof(cfg->bateria), "");
}

bool CEC_Validate(const cec_info_t* cfg)
{
    if (!cfg) return false;

    /* Regras mínimas: strings devem estar terminadas e não conter lixo (já garantido por SETs).
       Você pode endurecer as regras (ex: tamanho exato do SN) se quiser. */
    if (cfg->versao_fw[0] == '\0') return false;
    if (cfg->versao_hw[0] == '\0') return false;
    return true;
}

bool CEC_FromJsonString(const char* json, cec_info_t* out_cfg)
{
    if (!json || !out_cfg) return false;

    cJSON* root = cJSON_Parse(json);
    if (!root) return false;
    if (!cJSON_IsObject(root)) { cJSON_Delete(root); return false; }

    /* Preenche (se algum campo faltar, fica vazio) */
    (void)copy_str(out_cfg->versao_fw, sizeof(out_cfg->versao_fw), get_string_or_empty(root, "versao_fw"));
    (void)copy_str(out_cfg->versao_hw, sizeof(out_cfg->versao_hw), get_string_or_empty(root, "versao_hw"));
    (void)copy_str(out_cfg->data_ativacao, sizeof(out_cfg->data_ativacao), get_string_or_empty(root, "data_ativacao"));
    (void)copy_str(out_cfg->modelo, sizeof(out_cfg->modelo), get_string_or_empty(root, "modelo"));

    (void)copy_str(out_cfg->cec_sn, sizeof(out_cfg->cec_sn), get_string_or_empty(root, "cec_sn"));
    (void)copy_str(out_cfg->pws_sn, sizeof(out_cfg->pws_sn), get_string_or_empty(root, "pws_sn"));
    (void)copy_str(out_cfg->drv_sn, sizeof(out_cfg->drv_sn), get_string_or_empty(root, "drv_sn"));

    (void)copy_str(out_cfg->url, sizeof(out_cfg->url), get_string_or_empty(root, "url"));

    (void)copy_str(out_cfg->id_cliente, sizeof(out_cfg->id_cliente), get_string_or_empty(root, "id_cliente"));
    (void)copy_str(out_cfg->cliente, sizeof(out_cfg->cliente), get_string_or_empty(root, "cliente"));

    (void)copy_str(out_cfg->bateria, sizeof(out_cfg->bateria), get_string_or_empty(root, "bateria"));

    cJSON_Delete(root);
    return true;
}

char* CEC_ToJsonString(const cec_info_t* cfg)
{
    if (!cfg) return NULL;

    cJSON* root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "versao_fw", cfg->versao_fw);
    cJSON_AddStringToObject(root, "versao_hw", cfg->versao_hw);
    cJSON_AddStringToObject(root, "data_ativacao", cfg->data_ativacao);
    cJSON_AddStringToObject(root, "modelo", cfg->modelo);

    cJSON_AddStringToObject(root, "cec_sn", cfg->cec_sn);
    cJSON_AddStringToObject(root, "pws_sn", cfg->pws_sn);
    cJSON_AddStringToObject(root, "drv_sn", cfg->drv_sn);

    cJSON_AddStringToObject(root, "url", cfg->url);

    cJSON_AddStringToObject(root, "id_cliente", cfg->id_cliente);
    cJSON_AddStringToObject(root, "cliente", cfg->cliente);

    cJSON_AddStringToObject(root, "bateria", cfg->bateria);

    char* out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

void CEC_FreeJsonString(char* json_str)
{
    if (json_str) cJSON_free(json_str);
}

/* --------- GETs --------- */
#define DEF_GET(fn, field) const char* fn(const cec_info_t* cfg){ return cfg ? cfg->field : ""; }

DEF_GET(CEC_GetVersaoFW, versao_fw)
DEF_GET(CEC_GetVersaoHW, versao_hw)
DEF_GET(CEC_GetDataAtivacao, data_ativacao)
DEF_GET(CEC_GetModelo, modelo)
DEF_GET(CEC_GetCecSN, cec_sn)
DEF_GET(CEC_GetPwsSN, pws_sn)
DEF_GET(CEC_GetDrvSN, drv_sn)
DEF_GET(CEC_GetUrl, url)
DEF_GET(CEC_GetIdCliente, id_cliente)
DEF_GET(CEC_GetCliente, cliente)
DEF_GET(CEC_GetBateria, bateria)

/* --------- SETs --------- */
#define DEF_SET(fn, field) bool fn(cec_info_t* cfg, const char* s){ \
    if (!cfg || !s) return false; \
    return copy_str(cfg->field, sizeof(cfg->field), s); \
}

DEF_SET(CEC_SetVersaoFW, versao_fw)
DEF_SET(CEC_SetVersaoHW, versao_hw)
DEF_SET(CEC_SetDataAtivacao, data_ativacao)
DEF_SET(CEC_SetModelo, modelo)
DEF_SET(CEC_SetCecSN, cec_sn)
DEF_SET(CEC_SetPwsSN, pws_sn)
DEF_SET(CEC_SetDrvSN, drv_sn)
DEF_SET(CEC_SetUrl, url)
DEF_SET(CEC_SetIdCliente, id_cliente)
DEF_SET(CEC_SetCliente, cliente)
DEF_SET(CEC_SetBateria, bateria)
