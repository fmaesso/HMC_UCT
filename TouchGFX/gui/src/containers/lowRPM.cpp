#include <gui/containers/lowRPM.hpp>
#include "ui_data.h"

lowRPM::lowRPM(): lowRPMBtnCb(this, &lowRPM::onlowRPMButton)
{

}

void lowRPM::onlowRPMButton(const touchgfx::AbstractButton& btn){
	ResetaLowRPM();
}


void lowRPM::initialize()
{
    lowRPMBase::initialize();
    lowRPMBut.setAction(lowRPMBtnCb);
}
