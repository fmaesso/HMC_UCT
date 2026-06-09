#include <gui/runscreen_screen/RunScreenView.hpp>
#include "gui_generated/containers/erroRPMBase.hpp"

#include "ui_data.h"
#include "cec.h"

extern TShowTimers ShowTimers;
//extern TDataBarraSuperior DataBarraSuperior;
//extern TTelaPrincipal TelaPrincipal;


RunScreenView::RunScreenView():runTimer1Cb(this, &RunScreenView::runTimer1Button)
								,runTimer2Cb(this, &RunScreenView::runTimer2Button)
								,runTimer3Cb(this, &RunScreenView::runTimer3Button)
{

}

void RunScreenView::setupScreen()

{
    RunScreenViewBase::setupScreen();
    BTTim1.setAction(runTimer1Cb);
    BTTim2.setAction(runTimer2Cb);
    BTTim3.setAction(runTimer3Cb);
}

void RunScreenView::tearDownScreen()
{
    RunScreenViewBase::tearDownScreen();
}

void RunScreenView::gotoConfigScreen()
{
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())
        ->gotoConfigScreenScreenSlideTransitionEast();
}

void RunScreenView::updateBarraSuperior(const TDataBarraSuperior& d)
{
	barraConteiner1.setData(d);
}

void RunScreenView::updateClock(int totalSeconds){
	int hours = totalSeconds / 3600;
	int minutes = (totalSeconds % 3600) / 60;
	int seconds = totalSeconds % 60;

	// Formata a string com zeros à esquerda (02d)
	Unicode::snprintf(BufferComp, 15, "%02d:%02d:%02d", hours, minutes, seconds);
}

void RunScreenView::runTimer1Button(const touchgfx::AbstractButtonContainer& btn){

	if (ShowTimers.ShowTimerCountRun[0]){
		BTRed1.setVisible(true);
		BTGreen1.setVisible(false);
		TimerCount1.setColor(0xF7F2F2);
		ShowTimers.ShowTimerCountRun[0] = 0;
	}else{
		BTRed1.setVisible(false);
		BTGreen1.setVisible(true);
		if(ShowTimers.ShowTimerType[0]){
			TimerCount1.setColor(0x19A1E6);
		}else{
			TimerCount1.setColor(0xF0DA18);
		}
		ShowTimers.ShowTimerCountRun[0] = 1;
	}
	BTRed1.invalidate();
	BTGreen1.invalidate();
	TimerCount1.invalidate();
}

void RunScreenView::runTimer2Button(const touchgfx::AbstractButtonContainer& btn){
	if (ShowTimers.ShowTimerCountRun[1]){
		BTRed2.setVisible(true);
		BTGreen2.setVisible(false);
		TimerCount2.setColor(0xF7F2F2);
		ShowTimers.ShowTimerCountRun[1] = 0;
	}else{
		BTRed2.setVisible(false);
		BTGreen2.setVisible(true);
		if(ShowTimers.ShowTimerType[1]){
			TimerCount2.setColor(0x19A1E6);
		}else{
			TimerCount2.setColor(0xF0DA18);
		}
		ShowTimers.ShowTimerCountRun[1] = 1;
	}
	BTRed2.invalidate();
	BTGreen2.invalidate();
	TimerCount2.invalidate();
}

void RunScreenView::runTimer3Button(const touchgfx::AbstractButtonContainer& btn){
	if (ShowTimers.ShowTimerCountRun[2]){
		BTRed3.setVisible(true);
		BTGreen3.setVisible(false);
		TimerCount3.setColor(0xF7F2F2);
		ShowTimers.ShowTimerCountRun[2] = 0;
	}else{
		BTRed3.setVisible(false);
		BTGreen3.setVisible(true);
		if(ShowTimers.ShowTimerType[2]){
			TimerCount3.setColor(0x19A1E6);
		}else{
			TimerCount3.setColor(0xF0DA18);
		}
		ShowTimers.ShowTimerCountRun[2] = 1;
	}
	BTRed3.invalidate();
	BTGreen3.invalidate();
	TimerCount3.invalidate();
}


void RunScreenView::updateTelaPrincipal(const TTelaPrincipal& t)
{
	if(t.ErroRPM){
		if(!erroRPMCont.isVisible()){
			erroRPMCont.setVisible(true);
			erroRPMCont.invalidate();
		}
	}else{
		if(erroRPMCont.isVisible()){
			erroRPMCont.setVisible(false);
			erroRPMCont.invalidate();
		}
	}

	if(t.LowRPM){
		if(!lowRPM1.isVisible()){
			lowRPM1.setVisible(true);
			lowRPM1.invalidate();
		}
	}else{
		if(lowRPM1.isVisible()){
			lowRPM1.setVisible(false);
			lowRPM1.invalidate();
		}
	}

	Unicode::snprintfFloat(pressMedTextBuffer, 10, "%.1f", t.Ind_PressMed);
	pressMedText.setWildcard(pressMedTextBuffer);
	pressMedText.invalidate();

	Unicode::snprintfFloat(press1TextBuffer, 10, "%.1f", t.Ind_Press1);
	press1Text.setWildcard(press1TextBuffer);
	press1Text.invalidate();

	Unicode::snprintfFloat(press2TextBuffer, 10, "%.1f", t.Ind_Press2);
	press2Text.setWildcard(press2TextBuffer);
	press2Text.invalidate();


	Unicode::snprintfFloat(BufferComp, 10, "%.1f", t.Ind_Fluxo);
	if (Unicode::strncmp(BufferComp, fluxoTextBuffer, 10) != 0){
		Unicode::snprintfFloat(fluxoTextBuffer, 10, "%.1f", t.Ind_Fluxo);
		fluxoText.setWildcard(fluxoTextBuffer);
		fluxoText.invalidate();
	}


	Unicode::snprintf(BufferComp, 10, "%d", t.RPM);
	if (Unicode::strncmp(BufferComp, rpmTextBuffer, 10) != 0){
		Unicode::snprintf(rpmTextBuffer, 10, "%d", (int)t.RPM);
		rpmText.setWildcard(rpmTextBuffer);
		rpmText.invalidate();
	}

	if(t.TimerRun1){
		updateClock(t.Timer1);
		if (Unicode::strncmp(BufferComp, TimerCount1Buffer, 15) != 0){
			Unicode::strncpy(TimerCount1Buffer, BufferComp, 15);
			TimerCount1.setWildcard(TimerCount1Buffer);
			TimerCount1.invalidate();
		}
	}
	if(t.TimerRun2){
		updateClock(t.Timer2);
		if (Unicode::strncmp(BufferComp, TimerCount2Buffer, 15) != 0){
			Unicode::strncpy(TimerCount2Buffer, BufferComp, 15);
			TimerCount2.setWildcard(TimerCount2Buffer);
			TimerCount2.invalidate();
		}
	}
	if(t.TimerRun3){
		updateClock(t.Timer3);
		if (Unicode::strncmp(BufferComp, TimerCount3Buffer, 15) != 0){
			Unicode::strncpy(TimerCount3Buffer, BufferComp, 15);
			TimerCount3.setWildcard(TimerCount3Buffer);
			TimerCount3.invalidate();
		}
	}


}
