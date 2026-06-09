#include "main.h"
#include "cec.h"
#include "configs_json.h"
#include "cJSON.h"

#include <string.h>

/* ============================= small helpers ============================= */

static void safe_strcpy0(char *dst, size_t dst_sz, const char *src)
{
    if (!dst || dst_sz == 0) return;
    if (!src) { dst[0] = '\0'; return; }

    /* copy up to dst_sz-1 and always NUL-terminate */
    size_t n = strlen(src);
    if (n >= dst_sz) n = dst_sz - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static const cJSON* obj_get(const cJSON *obj, const char *key)
{
    if (!obj || !cJSON_IsObject(obj) || !key) return NULL;
    return cJSON_GetObjectItemCaseSensitive((cJSON*)obj, key);
}

static int read_string_field(const cJSON *obj, const char *key, char *dst, size_t dst_sz)
{
    const cJSON *it = obj_get(obj, key);
    if (!it) return 0; /* missing -> not an error */
    if (!cJSON_IsString(it) || it->valuestring == NULL) return -1;
    safe_strcpy0(dst, dst_sz, it->valuestring);
    return 1;
}

static int read_double_field(const cJSON *obj, const char *key, double *dst)
{
    const cJSON *it = obj_get(obj, key);
    if (!it) return 0;
    if (!cJSON_IsNumber(it)) return -1;
    *dst = it->valuedouble;
    return 1;
}

static int read_u32_field(const cJSON *obj, const char *key, uint32_t *dst)
{
    const cJSON *it = obj_get(obj, key);
    if (!it) return 0;
    if (!cJSON_IsNumber(it)) return -1;
    /* cJSON stores numbers as double; clamp to uint32_t domain if you want */
    if (it->valuedouble < 0) return -1;
    *dst = (uint32_t)(it->valuedouble);
    return 1;
}

static int read_int_field(const cJSON *obj, const char *key, int *dst)
{
    const cJSON *it = obj_get(obj, key);
    if (!it) return 0;
    if (cJSON_IsBool(it)) {
        *dst = cJSON_IsTrue(it) ? 1 : 0;
        return 1;
    }
    if (!cJSON_IsNumber(it)) return -1;
    *dst = (int)(it->valuedouble);
    return 1;
}

static int read_sentido_field(const cJSON *obj, const char *key, char *dst_char)
{
    const cJSON *it = obj_get(obj, key);
    if (!it) return 0;
    if (!cJSON_IsString(it) || it->valuestring == NULL) return -1;
    *dst_char = (it->valuestring[0] != '\0') ? it->valuestring[0] : '\0';
    return 1;
}

static int write_json_to_buffer(cJSON *root, char *out_json, size_t out_json_sz)
{
    if (!root || !out_json || out_json_sz == 0) return -1;

    char *printed = cJSON_PrintUnformatted(root); /* malloc inside cJSON */
    if (!printed) return -2;

    size_t need = strlen(printed); /* bytes excluding '\0' */
    if (need + 1 > out_json_sz) {  /* +1 for '\0' */
        cJSON_free(printed);
        if (out_json_sz) out_json[0] = '\0';
        return -3; /* buffer too small */
    }

    memcpy(out_json, printed, need + 1); /* include '\0' */

    /* sanity check: should end with '}' (or ']' if user changes root type) */
    if (need == 0 || (out_json[need - 1] != '}' && out_json[need - 1] != ']')) {
        cJSON_free(printed);
        out_json[0] = '\0';
        return -4;
    }

    cJSON_free(printed);
    return (int)need;
}

/* ============================ cec.json defaults =========================== */

void cec_info_defaults(cec_info_t *out)
{
    if (!out) return;

    /* defaults from example cec.json */
    safe_strcpy0(out->versao_fw,     sizeof(out->versao_fw),     "0.0.1");
    safe_strcpy0(out->versao_hw,     sizeof(out->versao_hw),     "1.0.0");
    safe_strcpy0(out->data_ativacao, sizeof(out->data_ativacao), "20/02/2026");
    safe_strcpy0(out->modelo,        sizeof(out->modelo),        "Hemocor-CEC V001.1");

    safe_strcpy0(out->cec_sn,  sizeof(out->cec_sn),  "D0642B6062EC");
    safe_strcpy0(out->pws_sn,  sizeof(out->pws_sn),  "D0642B6062EC");
    safe_strcpy0(out->drv_sn,  sizeof(out->drv_sn),  "D0642B6062EC");

    safe_strcpy0(out->url,     sizeof(out->url),     "http://teenpo.com.br/download");

    safe_strcpy0(out->id_cliente, sizeof(out->id_cliente), "123");
    safe_strcpy0(out->cliente,    sizeof(out->cliente),    "hemocor");

    safe_strcpy0(out->bateria, sizeof(out->bateria), "20/02/26");
}

int cec_info_from_json(cec_info_t *out, const char *cec_json)
{
    if (!out || !cec_json) return -1;

    cJSON *root = cJSON_Parse(cec_json);
    if (!root) return -2;
    if (!cJSON_IsObject(root)) { cJSON_Delete(root); return -3; }

    /* For each field: if present and valid -> update; if missing -> keep. */
    if (read_string_field(root, "versao_fw", out->versao_fw, sizeof(out->versao_fw)) < 0) { cJSON_Delete(root); return -10; }
    if (read_string_field(root, "versao_hw", out->versao_hw, sizeof(out->versao_hw)) < 0) { cJSON_Delete(root); return -11; }
    if (read_string_field(root, "data_ativacao", out->data_ativacao, sizeof(out->data_ativacao)) < 0) { cJSON_Delete(root); return -12; }
    if (read_string_field(root, "modelo", out->modelo, sizeof(out->modelo)) < 0) { cJSON_Delete(root); return -13; }

    if (read_string_field(root, "cec_sn", out->cec_sn, sizeof(out->cec_sn)) < 0) { cJSON_Delete(root); return -14; }
    if (read_string_field(root, "pws_sn", out->pws_sn, sizeof(out->pws_sn)) < 0) { cJSON_Delete(root); return -15; }
    if (read_string_field(root, "drv_sn", out->drv_sn, sizeof(out->drv_sn)) < 0) { cJSON_Delete(root); return -16; }

    if (read_string_field(root, "url", out->url, sizeof(out->url)) < 0) { cJSON_Delete(root); return -17; }

    if (read_string_field(root, "id_cliente", out->id_cliente, sizeof(out->id_cliente)) < 0) { cJSON_Delete(root); return -18; }
    if (read_string_field(root, "cliente", out->cliente, sizeof(out->cliente)) < 0) { cJSON_Delete(root); return -19; }

    if (read_string_field(root, "bateria", out->bateria, sizeof(out->bateria)) < 0) { cJSON_Delete(root); return -20; }

    cJSON_Delete(root);
    return 0;
}

int cec_info_to_json(const cec_info_t *in, char *out_json, size_t out_json_sz)
{
    if (!in || !out_json || out_json_sz == 0) return -1;

    cJSON *root = cJSON_CreateObject();
    if (!root) return -2;

    /* strings */
    cJSON_AddStringToObject(root, "versao_fw",     in->versao_fw);
    cJSON_AddStringToObject(root, "versao_hw",     in->versao_hw);
    cJSON_AddStringToObject(root, "data_ativacao", in->data_ativacao);
    cJSON_AddStringToObject(root, "modelo",        in->modelo);

    cJSON_AddStringToObject(root, "cec_sn", in->cec_sn);
    cJSON_AddStringToObject(root, "pws_sn", in->pws_sn);
    cJSON_AddStringToObject(root, "drv_sn", in->drv_sn);

    cJSON_AddStringToObject(root, "url", in->url);

    cJSON_AddStringToObject(root, "id_cliente", in->id_cliente);
    cJSON_AddStringToObject(root, "cliente",    in->cliente);

    cJSON_AddStringToObject(root, "bateria", in->bateria);

    int rc = write_json_to_buffer(root, out_json, out_json_sz);
    cJSON_Delete(root);
    return rc;
}

/* ========================= configs.json defaults ========================== */

void configs_defaults(configs_t *out)
{
    if (!out) return;

    /* defaults from example configs.json */
    out->pressao.Amin = 10.5;
    out->pressao.Amax = 16.5;
    out->pressao.Bmin = 120.5;
    out->pressao.Bmax = 140.5;

    out->fluxo.min = 5.0;
    out->fluxo.max = 7.0;

    out->rpm.min = 5.0;
    out->rpm.max = 7.0;

    for (int i = 0; i < 4; i++) {
        out->timer[i].tempo = 10000;
        out->timer[i].sentido = 'u';
        out->timer[i].alarme = 1;
    }
}

static int parse_range_obj(const cJSON *parent, const char *key, cfg_range_t *out)
{
    // pega objeto pelo nome
    const cJSON *o =
        cJSON_GetObjectItemCaseSensitive((cJSON*)parent, key);

    if (o == NULL)
        return 0;          // chave não existe

    if (!cJSON_IsObject(o))
        return -1;         // não é objeto

    // pega campos internos
    const cJSON *min =
        cJSON_GetObjectItemCaseSensitive((cJSON*)o, "min");

    const cJSON *max =
        cJSON_GetObjectItemCaseSensitive((cJSON*)o, "max");

    // valida tipos
    if (min && !cJSON_IsNumber(min))
        return -2;

    if (max && !cJSON_IsNumber(max))
        return -3;

    // copia valores (somente se existirem)
    if (min)
        out->min = min->valuedouble;

    if (max)
        out->max = max->valuedouble;

    return 1;
}

static int parse_pressao_obj(const cJSON *parent, const char *key, cfg_pressao_t *out)
{
    // 1) pega o objeto filho pelo nome (case-sensitive, como cJSON recomenda por padrão)
    const cJSON *o = cJSON_GetObjectItemCaseSensitive((cJSON*)parent, key);
    if (o == NULL) {
        return 0; // chave não existe: não é erro, só "não encontrado"
    }
    if (!cJSON_IsObject(o)) {
        return -1; // existe mas não é objeto
    }

    // 2) pega os campos internos (também case-sensitive)
    const cJSON *Amin = cJSON_GetObjectItemCaseSensitive((cJSON*)o, "Amin");
    const cJSON *Amax = cJSON_GetObjectItemCaseSensitive((cJSON*)o, "Amax");
    const cJSON *Bmin = cJSON_GetObjectItemCaseSensitive((cJSON*)o, "Bmin");
    const cJSON *Bmax = cJSON_GetObjectItemCaseSensitive((cJSON*)o, "Bmax");

    // 3) valida tipo (cJSON número = double internamente)
    if (Amin && !cJSON_IsNumber(Amin)) return -2;
    if (Amax && !cJSON_IsNumber(Amax)) return -3;
    if (Bmin && !cJSON_IsNumber(Bmin)) return -4;
    if (Bmax && !cJSON_IsNumber(Bmax)) return -5;

    // 4) aplica (se campo não existe, você decide: mantém valor anterior ou zera)
    if (Amin) out->Amin = (double)Amin->valuedouble;
    if (Amax) out->Amax = (double)Amax->valuedouble;
    if (Bmin) out->Bmin = (double)Bmin->valuedouble;
    if (Bmax) out->Bmax = (double)Bmax->valuedouble;

    return 1; // ok e encontrado
}

static int parse_timer_obj(const cJSON *parent, const char *key, cfg_timer_t *out)
{
    const cJSON *o = obj_get(parent, key);
    if (!o) return 0;
    if (!cJSON_IsObject(o)) return -1;

    int r;
    r = read_u32_field(o, "tempo", &out->tempo); if (r < 0) return -2;
    r = read_sentido_field(o, "sentido", &out->sentido); if (r < 0) return -3;
    r = read_int_field(o, "alarme", &out->alarme); if (r < 0) return -4;
    return 1;
}

int configs_from_json(configs_t *out, const char *configs_json)
{
    if (!out || !configs_json) return -1;

    cJSON *root = cJSON_Parse(configs_json);
    if (!root) return -2;
    if (!cJSON_IsObject(root)) { cJSON_Delete(root); return -3; }

    int r;

    r = parse_pressao_obj(root, "pressao", &out->pressao);
    if (r < 0) { cJSON_Delete(root); return -10; }

    r = parse_range_obj(root, "fluxo", &out->fluxo);
    if (r < 0) { cJSON_Delete(root); return -11; }

    /* Accept "RPM" (example) and "rpm" */
    r = parse_range_obj(root, "RPM", &out->rpm);
    if (r < 0) { cJSON_Delete(root); return -12; }
    if (r == 0) {
        r = parse_range_obj(root, "rpm", &out->rpm);
        if (r < 0) { cJSON_Delete(root); return -13; }
    }

    r = parse_timer_obj(root, "timer1", &out->timer[0]); if (r < 0) { cJSON_Delete(root); return -20; }
    r = parse_timer_obj(root, "timer2", &out->timer[1]); if (r < 0) { cJSON_Delete(root); return -21; }
    r = parse_timer_obj(root, "timer3", &out->timer[2]); if (r < 0) { cJSON_Delete(root); return -22; }
    r = parse_timer_obj(root, "timer4", &out->timer[3]); if (r < 0) { cJSON_Delete(root); return -23; }

    cJSON_Delete(root);
    return 0;
}

static cJSON* make_range_obj(const cfg_range_t *r)
{
    cJSON *o = cJSON_CreateObject();
    if (!o) return NULL;
    cJSON_AddNumberToObject(o, "min", r->min);
    cJSON_AddNumberToObject(o, "max", r->max);
    return o;
}

static cJSON* make_pressao_obj(const cfg_pressao_t *p)
{
    cJSON *o = cJSON_CreateObject();
    if (!o) return NULL;
    cJSON_AddNumberToObject(o, "Amin", p->Amin);
    cJSON_AddNumberToObject(o, "Amax", p->Amax);
    cJSON_AddNumberToObject(o, "Bmin", p->Bmin);
    cJSON_AddNumberToObject(o, "Bmax", p->Bmax);
    return o;
}

static cJSON* make_timer_obj(const cfg_timer_t *t)
{
    cJSON *o = cJSON_CreateObject();
    if (!o) return NULL;
    cJSON_AddNumberToObject(o, "tempo", (double)t->tempo);

    char sentido_str[2] = { t->sentido, '\0' };
    cJSON_AddStringToObject(o, "sentido", sentido_str);

    cJSON_AddNumberToObject(o, "alarme", (double)t->alarme);
    return o;
}

int configs_to_json(const configs_t *in, char *out_json, size_t out_json_sz)
{
    if (!in || !out_json || out_json_sz == 0) return -1;

    cJSON *root = cJSON_CreateObject();
    if (!root) return -2;

    cJSON *pressao = make_pressao_obj(&in->pressao);
    cJSON *fluxo   = make_range_obj(&in->fluxo);
    cJSON *rpm     = make_range_obj(&in->rpm);
    cJSON *t1      = make_timer_obj(&in->timer[0]);
    cJSON *t2      = make_timer_obj(&in->timer[1]);
    cJSON *t3      = make_timer_obj(&in->timer[2]);
    cJSON *t4      = make_timer_obj(&in->timer[3]);

    if (!pressao || !fluxo || !rpm || !t1 || !t2 || !t3 || !t4) {
        if (pressao) cJSON_Delete(pressao);
        if (fluxo)   cJSON_Delete(fluxo);
        if (rpm)     cJSON_Delete(rpm);
        if (t1)      cJSON_Delete(t1);
        if (t2)      cJSON_Delete(t2);
        if (t3)      cJSON_Delete(t3);
        if (t4)      cJSON_Delete(t4);
        cJSON_Delete(root);
        return -3;
    }

    cJSON_AddItemToObject(root, "pressao", pressao);
    cJSON_AddItemToObject(root, "fluxo",   fluxo);
    cJSON_AddItemToObject(root, "RPM",     rpm);     /* keep same key as example */
    cJSON_AddItemToObject(root, "timer1",  t1);
    cJSON_AddItemToObject(root, "timer2",  t2);
    cJSON_AddItemToObject(root, "timer3",  t3);
    cJSON_AddItemToObject(root, "timer4",  t4);

    int rc = write_json_to_buffer(root, out_json, out_json_sz);
    cJSON_Delete(root);
    return rc;
}
