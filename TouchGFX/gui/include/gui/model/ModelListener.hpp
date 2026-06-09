#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

#include "ui_data.h"

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    virtual void onTelaPrincipalUpdated(const TTelaPrincipal& t) {}
    virtual void onBarraSuperiorUpdated(const TDataBarraSuperior& t) {}
    virtual void onEncoderButtonPressed() {}

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
