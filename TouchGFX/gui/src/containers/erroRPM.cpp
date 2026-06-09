#include <gui/containers/erroRPM.hpp>
#include "ui_data.h"
erroRPM::erroRPM(): erroRPMBtnCb(this, &erroRPM::onErroRPMButton)
{

}

void erroRPM::initialize()
{
    erroRPMBase::initialize();
    erroRPMBut.setAction(erroRPMBtnCb);
}


void erroRPM::onErroRPMButton(const touchgfx::AbstractButton& btn){
	ResetaErroRPM();
}

//void RunScreenView::onErroRPMButton(const touchgfx::AbstractButton& btn){
//	erroRPMCont.setVisible(false);
//}
