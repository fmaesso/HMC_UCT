#ifndef INC_APP_UI_MODEL_H_
#define INC_APP_UI_MODEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_data.h"
#include <stdbool.h>

typedef enum {
    APP_UI_DATA_INVALID = 0,
    APP_UI_DATA_SIMULATED,
    APP_UI_DATA_VALID
} app_ui_data_quality_t;

typedef struct {
    TTelaPrincipal principal;
    TDataBarraSuperior barra;
    app_ui_data_quality_t flow_quality;
    app_ui_data_quality_t power_quality;
    bool hold_active;
} app_ui_snapshot_t;

void APP_UI_Model_GetSnapshot(app_ui_snapshot_t *out);
void APP_UI_Model_GetPrincipal(TTelaPrincipal *out);
void APP_UI_Model_GetBarra(TDataBarraSuperior *out);

#ifdef __cplusplus
}
#endif

#endif /* INC_APP_UI_MODEL_H_ */
