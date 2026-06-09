#include <gui/runscreen_screen/RunScreenView.hpp>
#include <gui/runscreen_screen/RunScreenPresenter.hpp>

#include "ui_data.h"


RunScreenPresenter::RunScreenPresenter(RunScreenView& v)
    : view(v)
{

}

void RunScreenPresenter::onEncoderButtonPressed()
{
    view.gotoConfigScreen();
}

void RunScreenPresenter::activate()
{

}

void RunScreenPresenter::deactivate()
{

}

void RunScreenPresenter::onTelaPrincipalUpdated(const TTelaPrincipal& t)
{
    view.updateTelaPrincipal(t);
}

void RunScreenPresenter::onBarraSuperiorUpdated(const TDataBarraSuperior& d)
{
    view.updateBarraSuperior(d);
}

