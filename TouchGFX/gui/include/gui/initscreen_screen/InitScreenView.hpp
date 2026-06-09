#ifndef INITSCREENVIEW_HPP
#define INITSCREENVIEW_HPP

#include <gui_generated/initscreen_screen/InitScreenViewBase.hpp>
#include <gui/initscreen_screen/InitScreenPresenter.hpp>

class InitScreenView : public InitScreenViewBase
{
public:
    InitScreenView();
    virtual ~InitScreenView() {}
    virtual void setupScreen();
    virtual void ShowInitMens();
    virtual void tearDownScreen();
private:
    char qrtext[150];
protected:
};

#endif // INITSCREENVIEW_HPP
