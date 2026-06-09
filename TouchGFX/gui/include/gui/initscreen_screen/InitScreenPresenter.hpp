#ifndef INITSCREENPRESENTER_HPP
#define INITSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class InitScreenView;

class InitScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    InitScreenPresenter(InitScreenView& v);

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

    virtual ~InitScreenPresenter() {}

private:
    InitScreenPresenter();

    InitScreenView& view;
};

#endif // INITSCREENPRESENTER_HPP
