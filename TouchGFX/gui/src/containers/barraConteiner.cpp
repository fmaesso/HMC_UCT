#include <gui/containers/barraConteiner.hpp>

#include <cstring>

#include "cec.h"

barraConteiner::barraConteiner()
{

}

void barraConteiner::initialize()
{
    barraConteinerBase::initialize();
}

void barraConteiner::setData(const TDataBarraSuperior & d)
{
	if (first || strcmp(d.Ind_DataHora, last.Ind_DataHora) != 0) {
		Unicode::fromUTF8(
		    (const uint8_t*)d.Ind_DataHora,
		    clockTextBuffer,
		    sizeof(clockTextBuffer) / sizeof(clockTextBuffer[0])
		);
//		Unicode::snprintf(clockTextBuffer, 9, "%s", d.Ind_DataHora);
		clockText.setWildcard(clockTextBuffer);
		clockText.invalidate();
    }

    if (first || d.Ind_Bateria != last.Ind_Bateria) {
	        Unicode::snprintf(batTextBuffer, 5, "%d", d.Ind_Bateria);
	        batText.setWildcard(batTextBuffer);
	        batText.invalidate();
    }

    last = d;
    first = false;
}

void barraConteiner::btnPlusClicked()
{
    ExecLedDisplayPlus();
}

void barraConteiner::btnMinusClicked()
{
    ExecLedDisplayMinus();
}
