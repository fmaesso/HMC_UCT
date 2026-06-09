#ifndef RUNSCREENVIEW_HPP
#define RUNSCREENVIEW_HPP

#include <gui_generated/runscreen_screen/RunScreenViewBase.hpp>
#include <gui/runscreen_screen/RunScreenPresenter.hpp>
#include "ui_data.h"



class RunScreenView : public RunScreenViewBase
{
public:
    RunScreenView();
    virtual ~RunScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void updateTelaPrincipal(const TTelaPrincipal& t);
    virtual void updateBarraSuperior(const TDataBarraSuperior& d);
    virtual void gotoConfigScreen();
protected:
    void updateClock(int totalSeconds);


    touchgfx::Unicode::UnicodeChar press1TextBuffer[10];
    touchgfx::Unicode::UnicodeChar press2TextBuffer[10];
    touchgfx::Unicode::UnicodeChar fluxoTextBuffer[10];
    touchgfx::Unicode::UnicodeChar rpmTextBuffer[10];

    touchgfx::Unicode::UnicodeChar BufferComp[10];

    touchgfx::Unicode::UnicodeChar clockTextBuffer[10];
    touchgfx::Unicode::UnicodeChar rpmTextBenergiaTextBufferuffer[10];


	touchgfx::Callback<RunScreenView, const touchgfx::AbstractButtonContainer&> runTimer1Cb;
	void runTimer1Button(const touchgfx::AbstractButtonContainer& btn);

	touchgfx::Callback<RunScreenView, const touchgfx::AbstractButtonContainer&> runTimer2Cb;
	void runTimer2Button(const touchgfx::AbstractButtonContainer& btn);

	touchgfx::Callback<RunScreenView, const touchgfx::AbstractButtonContainer&> runTimer3Cb;
	void runTimer3Button(const touchgfx::AbstractButtonContainer& btn);


};

#endif // RUNSCREENVIEW_HPP
