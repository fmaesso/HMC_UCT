#ifndef DEVICE_JSON_CJSON_H
#define DEVICE_JSON_CJSON_H

/*
 * JSON <-> struct helpers using DaveGamble cJSON (cJSON.c / cJSON.h).
 *
 * This module defines 6 functions:
 *  - 3 for cec.json <-> cec_info_t
 *  - 3 for configs.json <-> configs_t
 *
 * It expects the following structs (from your struct.txt):
 *   - cec_info_t
 *   - configs_t (with cfg_pressao_t, cfg_range_t, cfg_timer_t)
 *
 * If you already define these structs elsewhere, define
 *   DEVICE_JSON_CJSON_EXTERNAL_STRUCTS
 * before including this header, and include your struct header first.
 */

#include <stddef.h>
#include <stdint.h>
#include "cec.h"
#ifdef __cplusplus
extern "C" {
#endif


/* ========================= cec.json (cec_info_t) ========================= */

void cec_info_defaults(cec_info_t *out);

/* Parse JSON text (cec.json format) into struct.
 * Returns 0 on success, <0 on error.
 * - Missing fields keep current values already in *out.
 * - Unknown fields are ignored.
 */
int  cec_info_from_json(cec_info_t *out, const char *cec_json);

/* Serialize struct into JSON text (cec.json format).
 * out_json is provided by the caller; out_json_sz is its total size in bytes.
 * Returns number of bytes written (excluding the final '\0') on success, <0 on error.
 */
int  cec_info_to_json(const cec_info_t *in, char *out_json, size_t out_json_sz);


/* ======================= configs.json (configs_t) ========================= */

void configs_defaults(configs_t *out);

/* Parse JSON text (configs.json format) into struct.
 * Returns 0 on success, <0 on error.
 * - Missing fields keep current values already in *out.
 * - Unknown fields are ignored.
 * Notes:
 * - Accepts "RPM" (as in example) and also "rpm".
 * - "sentido" is expected to be a string; the first character is used.
 */
int  configs_from_json(configs_t *out, const char *configs_json);

/* Serialize struct into JSON text (configs.json format).
 * out_json is provided by the caller; out_json_sz is its total size in bytes.
 * Returns number of bytes written (excluding the final '\0') on success, <0 on error.
 */
int  configs_to_json(const configs_t *in, char *out_json, size_t out_json_sz);


#ifdef __cplusplus
}
#endif

#endif /* DEVICE_JSON_CJSON_H */
