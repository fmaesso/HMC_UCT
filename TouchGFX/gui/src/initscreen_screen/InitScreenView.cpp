#include <gui/initscreen_screen/InitScreenView.hpp>
#include "cec.h"
#include "ui_data.h"
#include <stdio.h>
#include <cstdio>
#include <touchgfx/Unicode.hpp>
#include <cstring>   // se usar strlen

InitScreenView::InitScreenView()
{

}

void InitScreenView::ShowInitMens(){
	cec_info_t c;

	getDataCECTCX(&c);

	touchgfx::Unicode::strncpy(swSNBuffer, c.cec_sn, 16);
	swSN.setWildcard(swSNBuffer);
	swSN.invalidate();

	touchgfx::Unicode::strncpy(swModelBuffer, c.modelo, 16);
	swModel.setWildcard(swModelBuffer);
	swModel.invalidate();

	touchgfx::Unicode::strncpy(swFHBuffer, c.versao_fw, 16);
	swFH.setWildcard(swFHBuffer);
	swFH.invalidate();

	touchgfx::Unicode::strncpy(swHwBuffer, c.versao_hw, 16);
	swHw.setWildcard(swHwBuffer);
	swHw.invalidate();

//	Unicode::snprintf(valTimer1Buffer, 16, "%1f", d.timer[0].tempo);

	std::snprintf(qrtext, sizeof(qrtext), "%s/%s", c.url, c.cec_sn);
	qrCode1.convertStringToQRCode(qrtext);
	qrCode1.invalidate();

}

void InitScreenView::setupScreen()
{
    InitScreenViewBase::setupScreen();
    ShowInitMens();
}

void InitScreenView::tearDownScreen()
{
    InitScreenViewBase::tearDownScreen();
}
