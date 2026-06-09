#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C"
{
#include "ui_data.h"
#include "encoder.h"
#include "cec.h"
}

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
	TTelaPrincipal t;
	TDataBarraSuperior b;

//escuta a interrpcao do encoder
	if (EncoderSW_GetAndClearEvent()){
		if (modelListener) {
			modelListener->onEncoderButtonPressed();
	    }
	}

	ui_getdataPrincipal(&t);
	ui_getdataBarra(&b);

	if (modelListener) {
		modelListener->onTelaPrincipalUpdated(t);
		modelListener->onBarraSuperiorUpdated(b);
	}
}
