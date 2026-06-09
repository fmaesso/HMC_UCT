#ifndef SOBRESCREENVIEW_HPP
#define SOBRESCREENVIEW_HPP

#include <gui_generated/sobrescreen_screen/SobreScreenViewBase.hpp>
#include <gui/sobrescreen_screen/SobreScreenPresenter.hpp>

class SobreScreenView : public SobreScreenViewBase
{
public:
    SobreScreenView();
    virtual ~SobreScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // SOBRESCREENVIEW_HPP
