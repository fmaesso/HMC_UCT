#ifndef SOBRESCREENPRESENTER_HPP
#define SOBRESCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class SobreScreenView;

class SobreScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    SobreScreenPresenter(SobreScreenView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~SobreScreenPresenter() {}

private:
    SobreScreenPresenter();

    SobreScreenView& view;
};

#endif // SOBRESCREENPRESENTER_HPP
