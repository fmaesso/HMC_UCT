#include <gui/configscreen_screen/ConfigScreenView.hpp>
#include <gui/configscreen_screen/ConfigScreenPresenter.hpp>

ConfigScreenPresenter::ConfigScreenPresenter(ConfigScreenView& v)
    : view(v)
{

}

void ConfigScreenPresenter::onBarraSuperiorUpdated(const TDataBarraSuperior& d)
{
    view.updateBarraSuperior(d);
}


void ConfigScreenPresenter::activate()
{

}

void ConfigScreenPresenter::deactivate()
{

}
