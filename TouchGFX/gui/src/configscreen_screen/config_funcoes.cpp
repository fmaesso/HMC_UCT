/*
 * config_funcoes.cpp
 *
 *  Created on: 21 de fev. de 2026
 *      Author: ferna
 */

#include <gui/configscreen_screen/ConfigScreenView.hpp>
#include <ctype.h>

extern "C" {
	#include "drv_rtc.h"
	#include "cec.h"
	#include "ui_data.h"
}

// Converte UnicodeChar buffer para float (aceita "12.3" e "12,3")
static float u16_to_float(const touchgfx::Unicode::UnicodeChar* u16)
{
    if (!u16) return 0.0f;

    char tmp[32];
    // copia para ASCII
    touchgfx::Unicode::toUTF8(u16, (uint8_t*)tmp, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';

    // troca ',' por '.'
    for (size_t i=0; tmp[i]; i++) {
        if (tmp[i] == ',') tmp[i] = '.';
    }

    return (float)strtod(tmp, NULL);
}

static uint32_t u16_to_u32(const touchgfx::Unicode::UnicodeChar* u16)
{
    if (!u16) return 0;

    char tmp[32];
    touchgfx::Unicode::toUTF8(u16, (uint8_t*)tmp, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';

    // pula espaços
    char* p = tmp;
    while (*p && isspace((unsigned char)*p)) p++;

    return (uint32_t)strtoul(p, NULL, 10);
}


void ConfigScreenView::SetaWheelClock(){

    // 3) Configura wheels
	wheelHour.setNumberOfItems(24);
	wheelMin.setNumberOfItems(60);
	wheelDay.setNumberOfItems(31);
	wheelMonth.setNumberOfItems(12);
	wheelYear.setNumberOfItems(40);

	// 4) Inicializa wheels com RTC atual (se driver existir)
	rtc_datetime_t dt;
	if (DRV_RTC_GetDateTime(&dt))
	{
	    selHour = (uint8_t)dt.hour;
	    selMin  = (uint8_t)dt.min;
	    selDia =  (uint8_t)dt.day;
	    selMes =  (uint8_t)dt.month;
	    selAno =  (uint8_t)dt.year;
	}else{
	    selHour = 12;
	    selMin  = 0;
	    selDia =  1;
	    selMes =  1;
	    selAno =  26;
	}
	wheelHour.invalidate();
	wheelMin.invalidate();
	wheelDay.invalidate();
	wheelMonth.invalidate();
	wheelYear.invalidate();

	wheelHour.animateToItem(selHour, 0);
	wheelMin.animateToItem(selMin, 0);
	wheelDay.animateToItem(selDia, 0);
	wheelMonth.animateToItem(selMes, 0);
	wheelYear.animateToItem(selAno, 0);
	// 5) Conecta botão “Aplicar” (Base não criou handler automático)
	applyButton.setAction(applyBtnCb);
}



void ConfigScreenView::getValoresConfig(){
	getDataConfigTCX(&cp_cfg);
//	Unicode::snprintfFloat(press1TextBuffer, 10, "%.1f", t.Ind_Press1);
	Unicode::snprintfFloat(FluxoMinBuffer, 16, "%.1f", cp_cfg.fluxo.min);
	FluxoMin.setWildcard(FluxoMinBuffer);
	FluxoMin.invalidate();

	Unicode::snprintfFloat(FluxoMaxBuffer, 16, "%.1f", cp_cfg.fluxo.max);
	FluxoMax.setWildcard(FluxoMaxBuffer);
	FluxoMax.invalidate();

	Unicode::snprintfFloat(valRPMMinBuffer, 16, "%.1f", cp_cfg.rpm.min);
	valRPMMin.setWildcard(valRPMMinBuffer);
	valRPMMin.invalidate();

	Unicode::snprintfFloat(valRPMMaxBuffer, 16, "%.1f", cp_cfg.rpm.max);
	valRPMMax.setWildcard(valRPMMaxBuffer);
	valRPMMax.invalidate();

	Unicode::snprintfFloat(PressAMinBuffer, 16, "%.1f", cp_cfg.pressao.Amin);
	PressAMin.setWildcard(PressAMinBuffer);
	PressAMin.invalidate();

	Unicode::snprintfFloat(PressAMaxBuffer, 16, "%.1f", cp_cfg.pressao.Amax);
	PressAMax.setWildcard(PressAMaxBuffer);
	PressAMax.invalidate();

	Unicode::snprintfFloat(PressBMinBuffer, 16, "%.1f", cp_cfg.pressao.Bmin);
	PressBMin.setWildcard(PressBMinBuffer);
	PressBMin.invalidate();

	Unicode::snprintfFloat(valPressBMaxBuffer, 16, "%.1f", cp_cfg.pressao.Bmax);
	valPressBMax.setWildcard(valPressBMaxBuffer);
	valPressBMax.invalidate();

	Unicode::snprintf(valTimer1Buffer, 16, "%d", cp_cfg.timer[0].tempo);
	valTimer1.setWildcard(valTimer1Buffer);
	valTimer1.invalidate();

	Unicode::snprintf(valTimer2Buffer, 16, "%d", cp_cfg.timer[1].tempo);
	valTimer2.setWildcard(valTimer2Buffer);
	valTimer2.invalidate();

	Unicode::snprintf(valTimer3Buffer, 16, "%d", cp_cfg.timer[2].tempo);
	valTimer3.setWildcard(valTimer3Buffer);
	valTimer3.invalidate();

	Unicode::snprintf(valTimer4Buffer, 16, "%d", cp_cfg.timer[3].tempo);
	valTimer4.setWildcard(valTimer4Buffer);
	valTimer4.invalidate();

	rbTim1.setSelected(false);
	rbTim2.setSelected(false);
	rbTim3.setSelected(false);
	rbTim4.setSelected(false);

	if(cp_cfg.timer[0].sentido == 'u'){
		rbTim1.setSelected(true);
		rbTim1.invalidate();
	}
	if(cp_cfg.timer[1].sentido == 'u'){
		rbTim2.setSelected(true);
		rbTim2.invalidate();
	}
	if(cp_cfg.timer[2].sentido == 'u'){
		rbTim3.setSelected(true);
		rbTim3.invalidate();
	}
	if(cp_cfg.timer[3].sentido == 'u'){
		rbTim4.setSelected(true);
		rbTim4.invalidate();
	}


}


void ConfigScreenView::SetValoresConfig()
{
    // 1) Campos numéricos (buffers -> struct)

    cp_cfg.fluxo.min      = (double)u16_to_float(FluxoMinBuffer);
    cp_cfg.fluxo.max      = (double)u16_to_float(FluxoMaxBuffer);

    cp_cfg.rpm.min        = (double)u16_to_float(valRPMMinBuffer);
    cp_cfg.rpm.max        = (double)u16_to_float(valRPMMaxBuffer);

    cp_cfg.pressao.Amin   = (double)u16_to_float(PressAMinBuffer);
    cp_cfg.pressao.Amax   = (double)u16_to_float(PressAMaxBuffer);
    cp_cfg.pressao.Bmin   = (double)u16_to_float(PressBMinBuffer);
    cp_cfg.pressao.Bmax   = (double)u16_to_float(valPressBMaxBuffer);

    // timers (ms)
    cp_cfg.timer[0].tempo = u16_to_u32(valTimer1Buffer);
    cp_cfg.timer[1].tempo = u16_to_u32(valTimer2Buffer);
    cp_cfg.timer[2].tempo = u16_to_u32(valTimer3Buffer);
    cp_cfg.timer[3].tempo = u16_to_u32(valTimer4Buffer);

    // 2) RadioButtons -> sentido
    // Pelo seu código: “selecionado = sentido 'u'”, não selecionado = 'd' (ou outro)
    // Ajuste o else para o padrão que você quer.
    cp_cfg.timer[0].sentido = rbTim1.getSelected() ? 'u' : 'd';
    cp_cfg.timer[1].sentido = rbTim2.getSelected() ? 'u' : 'd';
    cp_cfg.timer[2].sentido = rbTim3.getSelected() ? 'u' : 'd';
    cp_cfg.timer[3].sentido = rbTim4.getSelected() ? 'u' : 'd';

    // 3) (Opcional) alarmes (se tiver checkbox/switch, faça aqui)
    // cp_cfg.timer[i].alarme = ...

    // 4) Agora você chama sua função de salvar (exemplo)
    setDataConfigTCX(&cp_cfg);  // ou presenter->saveConfigs(cp_cfg);
}

