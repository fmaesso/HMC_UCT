#ifndef RUNSCREENPRESENTER_HPP
#define RUNSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>
#include "ui_data.h"

using namespace touchgfx;

class RunScreenView;

class RunScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    RunScreenPresenter(RunScreenView& v);
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

    virtual void onTelaPrincipalUpdated(const TTelaPrincipal& t) override;
    virtual void onBarraSuperiorUpdated(const TDataBarraSuperior& d) override;
    virtual void onEncoderButtonPressed();

    virtual ~RunScreenPresenter() {}

private:
    RunScreenPresenter();

    RunScreenView& view;
};

#endif // RUNSCREENPRESENTER_HPP
