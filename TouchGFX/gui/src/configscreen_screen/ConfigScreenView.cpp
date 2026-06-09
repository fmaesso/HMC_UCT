#include <gui/configscreen_screen/ConfigScreenView.hpp>
#include <touchgfx/Color.hpp>


struct TMenuItem
{
    uint16_t id;
    const char* title;
};

static const TMenuItem menuItems[] =
{
    {0, "Limites de Pressão"},
    {1, "Limites de Fluxo"},
    {2, "Limites de RPM"},
    {3, "Temporizadores"},
    {4, "Energia e Bateria"},
    {5, "Data e Hora"},
    {6, "Relatórios de Ciclos"},
    {7, "Testes de Hardware"},
    {8, "Sair"},
};

static const int MENU_COUNT = 9;

extern "C" {
	#include "drv_rtc.h"
	#include "cec.h"
}

ConfigScreenView::ConfigScreenView()
		:scrollList2ItemSelectedCallback(this, &ConfigScreenView::scrollList2ItemSelectedHandler)
		 ,applyBtnCb(this, &ConfigScreenView::onApplyButton)
		 ,saveBtnCb(this, &ConfigScreenView::onSaveButton)
		 ,tecladoOkCb(this, &ConfigScreenView::tecladoOkHandler)
		 ,fbCb(this, &ConfigScreenView::fbHandler)
//		itemCb(this, &ConfigScreenView::onMenuClicked)
{

}


void ConfigScreenView::SetaTeclado(){
	teclado1Container.setVisible(false);
	teclado1Container.setOkCallback(&tecladoOkCb);
	teclado1Container.setKeyRepeatTicks(10);
}

void ConfigScreenView::BotoesEdicao(){
	// registra callbacks dos botões transparentes
	fbPressAMin.setAction(fbCb);
	fbPressAMax.setAction(fbCb);
	fbPressBMin.setAction(fbCb);
//	fbPressBMax.setAction(fbCb);
	fbPressB2.setAction(fbCb);

	fbFluxoMin.setAction(fbCb);
	fbFluxoMax.setAction(fbCb);

	fbTim1.setAction(fbCb);
	fbTim2.setAction(fbCb);
	fbTim3.setAction(fbCb);
	fbTim4.setAction(fbCb);

	fbRPMMin.setAction(fbCb);
	fbRPMMax.setAction(fbCb);

	activeText = nullptr;
	normalColor = PressAMin.getColor();      // assume que todos usam a mesma cor normal
	editColor   = touchgfx::Color::getColorFromRGB(255, 0, 0); // amarelo por exemplo
}



void ConfigScreenView::setupScreen()
{
    ConfigScreenViewBase::setupScreen();
    scrollList2.setItemSelectedCallback(scrollList2ItemSelectedCallback);
    saveButton.setAction(saveBtnCb);

    // GARANTE que cada TextArea aponta pro seu próprio buffer
	PressAMin.setWildcard(PressAMinBuffer);
	PressAMax.setWildcard(PressAMaxBuffer);
	PressBMin.setWildcard(PressBMinBuffer);
//	PressBMax.setWildcard(PressBMaxBuffer);
	valPressBMax.setWildcard(valPressBMaxBuffer);

	// Faça o mesmo para RPM (ajuste os nomes exatos)
	valRPMMin.setWildcard(valRPMMinBuffer);
	valRPMMax.setWildcard(valRPMMaxBuffer);

	// (opcional) atualiza visual imediatamente
	PressAMin.invalidate();
	PressAMax.invalidate();
	PressBMin.invalidate();
//	PressBMax.invalidate();
	valRPMMin.invalidate();
	valRPMMax.invalidate();


    SetaWheelClock();
    getValoresConfig();
    SetaTeclado();
    BotoesEdicao();
    savedContainer1.setVisible(false);
    DatHoraContainer.setVisible(false);
    FluxoContainer.setVisible(false);
    RPMContainer.setVisible(false);
 	TimersContainer.setVisible(false);
    PressaoContainer.setVisible(true);
    PressaoContainer.invalidate();
}

void ConfigScreenView::wheelMonthUpdateItem(DataHoraContainer& item, int16_t itemIndex){
    // itemIndex vem de 0..12
    if (itemIndex < 0) return;
    item.set2digits((uint8_t)itemIndex);
}

void ConfigScreenView::wheelDayUpdateItem(DataHoraContainer& item, int16_t itemIndex){
    // itemIndex vem de 0..31
    if (itemIndex < 0) return;
    item.set2digits((uint8_t)itemIndex);
}

void ConfigScreenView::wheelYearUpdateItem(DataHoraContainer& item, int16_t itemIndex){
    // itemIndex vem de 0..40
    if (itemIndex < 0) return;
    item.set2digits((uint8_t)itemIndex);
}

// ===== Wheel hour update (item do wheel = DataHoraContainer) =====
void ConfigScreenView::wheelHourUpdateItem(DataHoraContainer& item, int16_t itemIndex)
{
    // itemIndex vem de 0..23
    if (itemIndex < 0) return;
    item.set2digits((uint8_t)itemIndex);
}

// ===== Wheel min update (item do wheel = DataHoraContainer) =====
void ConfigScreenView::wheelMinUpdateItem(DataHoraContainer& item, int16_t itemIndex)
{
    // itemIndex vem de 0..59
    if (itemIndex < 0) return;
    item.set2digits((uint8_t)itemIndex);
}


void ConfigScreenView::onSaveButton(const touchgfx::AbstractButton& btn){
    (void)btn;

    SetValoresConfig();

    DatHoraContainer.setVisible(false);
    FluxoContainer.setVisible(false);
    RPMContainer.setVisible(false);
 	TimersContainer.setVisible(false);
    PressaoContainer.setVisible(false);
    savedContainer1.setVisible(true);
    savedContainer1.invalidate();

}


// ===== Apply button callback =====
void ConfigScreenView::onApplyButton(const touchgfx::AbstractButton& btn)
{
    (void)btn;

    // Lê valores escolhidos nos wheels
    selHour = (uint8_t)wheelHour.getSelectedItem();
    selMin  = (uint8_t)wheelMin.getSelectedItem();
    selDia  = (uint8_t)wheelDay.getSelectedItem();
    selMes  = (uint8_t)wheelMonth.getSelectedItem();
    selAno  = (uint8_t)wheelYear.getSelectedItem();

    rtc_datetime_t dt;
    if (!DRV_RTC_GetDateTime(&dt))
    {
        // fallback mínimo se leitura falhar
        // ajuste campos obrigatórios do seu rtc_datetime_t se existirem
        dt.hour = selHour;
        dt.min = selMin;
        dt.day = selDia;
        dt.month = selMes;
        dt.year = selAno;
        dt.sec = 0;
        (void)DRV_RTC_SetDateTime(&dt);
    }
    else
    {
        dt.hour = selHour;
        dt.min = selMin;
        dt.sec = 0; // opcional: zera segundos ao aplicar
        dt.day = selDia;
        dt.month = selMes;
        dt.year = selAno;
        (void)DRV_RTC_SetDateTime(&dt);
    }

    // Feedback no MenuSelect
//    Unicode::snprintf(MenuSelectBuffer, MENUSELECT_SIZE, "RTC %02d:%02d aplicado", selHour, selMin);
//    MenuSelect.invalidate();
}

void ConfigScreenView::scrollList2ItemSelectedHandler(int16_t itemSelected){
	if (itemSelected < 0 || itemSelected >= MENU_COUNT)
	        return;
	Unicode::fromUTF8(
	        (const uint8_t*)menuItems[itemSelected].title,
	        MenuSelectBuffer,
	        MENUSELECT_SIZE
	    );
	MenuSelect.invalidate();

    DatHoraContainer.setVisible(false);
    FluxoContainer.setVisible(false);
    PressaoContainer.setVisible(false);
    TimersContainer.setVisible(false);
    RPMContainer.setVisible(false);

    switch (itemSelected)
        {
        case 0:
        	PressaoContainer.setVisible(true);
        	PressaoContainer.invalidate();
            break;

        case 1:
        	FluxoContainer.setVisible(true);
        	FluxoContainer.invalidate();
            break;

        case 2:
        	RPMContainer.setVisible(true);
        	RPMContainer.invalidate();
            break;

        case 3:
        	TimersContainer.setVisible(true);
        	TimersContainer.invalidate();
            break;

        case 5:
        	DatHoraContainer.setVisible(true);
        	DatHoraContainer.invalidate();
            break;
        }
}

void ConfigScreenView::scrollList2UpdateItem(ItemMenu& itemM, int16_t itemIndex)
{
    if (itemIndex < 0 || itemIndex >= MENU_COUNT)
        return;

    // Atualiza texto e também guarda o índice dentro do item
    itemM.setIndex(menuItems[itemIndex].id);
    itemM.setTitle(menuItems[itemIndex].title);
}

void ConfigScreenView::onMenuClicked(uint16_t idx)
{
    // abrir submenu/tela conforme idx
}


/////////////////////////////////////////////////////////////////////////////////////////
void ConfigScreenView::openKeyboardFor(touchgfx::TextAreaWithOneWildcard& ta,
                                      touchgfx::Unicode::UnicodeChar* buf,
                                      uint16_t bufSize,
                                      int16_t x, int16_t y, int16_t max)
{
	// restaura anterior
	if (activeText)
	{
		activeText->setColor(normalColor);
		activeText->invalidate();
	}

	activeText = &ta;
	activeText->setColor(editColor);
	activeText->invalidate();

	// posiciona onde você quiser
	teclado1Container.setXY(x, y);

    // aponta o teclado para o campo/buffer
	teclado1Container.setTarget(&ta, buf, bufSize, max);

    // mostra
	teclado1Container.setVisible(true);
	teclado1Container.invalidate();
}

void ConfigScreenView::fbHandler(const touchgfx::AbstractButtonContainer& src)
{
    // escolha as posições que você quiser para cada campo (ou uma mesma posição)
    const int16_t KX = 720;
    const int16_t KY = 70;
    commitEditingIfNeeded();

    if (&src == &fbPressAMin){
        openKeyboardFor(PressAMin, PressAMinBuffer, PRESSAMIN_SIZE, KX, KY, 6);
    }
    else if (&src == &fbPressAMax){
        openKeyboardFor(PressAMax, PressAMaxBuffer, PRESSAMAX_SIZE, KX, KY, 6);
    }
    else if (&src == &fbPressBMin){
        openKeyboardFor(PressBMin, PressBMinBuffer, PRESSBMIN_SIZE, KX, KY, 6);
    }
//    else if (&src == &fbPressBMax){
//        openKeyboardFor(PressBMax, PressBMaxBuffer, PRESSBMAX_SIZE, KX, KY, 6);
//    }
//
    else if (&src == &fbPressB2){
        openKeyboardFor(valPressBMax, valPressBMaxBuffer, VALPRESSBMAX_SIZE, KX, KY, 6);
    }


    else if (&src == &fbFluxoMin){
        openKeyboardFor(FluxoMin, FluxoMinBuffer, FLUXOMIN_SIZE, KX, KY, 6);
    }
    else if (&src == &fbFluxoMax){
        openKeyboardFor(FluxoMax, FluxoMaxBuffer, FLUXOMAX_SIZE, KX, KY, 6);
    }

    else if (&src == &fbTim1){
        openKeyboardFor(valTimer1, valTimer1Buffer, VALTIMER1_SIZE, KX, KY, 6);
    }

    else if (&src == &fbTim2){
        openKeyboardFor(valTimer2, valTimer2Buffer, VALTIMER2_SIZE, KX, KY, 6);
    }

    else if (&src == &fbTim3){
        openKeyboardFor(valTimer3, valTimer3Buffer, VALTIMER3_SIZE, KX, KY, 6);
    }

    else if (&src == &fbTim4){
        openKeyboardFor(valTimer4, valTimer4Buffer, VALTIMER4_SIZE, KX, KY, 6);
    }

    else if (&src == &fbRPMMin){
        openKeyboardFor(valRPMMin, valRPMMinBuffer, VALRPMMIN_SIZE, KX, KY, 6);
    }

    else if (&src == &fbRPMMax){
        openKeyboardFor(valRPMMax, valRPMMaxBuffer, VALRPMMAX_SIZE, KX, KY, 6);
    }

//    fbFluxoMin.setAction(fbCb);
//	fbFluxoMax.setAction(fbCb);
//	fbTim1.setAction(fbCb);
//	fbTim2.setAction(fbCb);
//	fbTim3.setAction(fbCb);
//	fbTim4.setAction(fbCb);
//
//	fbRPMMin.setAction(fbCb);
//	fbRPMMax.setAction(fbCb);


}

void ConfigScreenView::closeKeyboardUiOnly()
{
    // só fecha UI, não mexe em texto (o texto já está no buffer)
	teclado1Container.setVisible(false);
	teclado1Container.invalidate();
	teclado1Container.clearTarget();

    // tira cor de edição (se você usa)
    if (activeText)
    {
        activeText->setColor(normalColor);
        activeText->invalidate();
        activeText = nullptr;
    }
}

void ConfigScreenView::commitEditingIfNeeded()
{
    if (!teclado1Container.isVisible())
        return;

    // Aqui é o “OK automático”:
    // Se você tem Presenter e quer gravar o valor, chame aqui.
    // Exemplo (opcional):
    // presenter->setPressAMin( parseBuffer(PressAMinBuffer) );

    closeKeyboardUiOnly();
}

void ConfigScreenView::tecladoCancelHandler(const TecladoContainer&)
{
}

void ConfigScreenView::tecladoOkHandler(const TecladoContainer&)
{
    // Fecha teclado
	teclado1Container.setVisible(false);
    teclado1Container.invalidate();

    // opcional: limpar target
    teclado1Container.clearTarget();
    if (activeText)
        {
            activeText->setColor(normalColor);
            activeText->invalidate();
            activeText = nullptr;
        }
    // opcional: aqui você pode validar/converter (min/max) e formatar
    // ex: presenter->setPressAMin( atof(...) ), etc.
}

void ConfigScreenView::handleTickEvent()
{
    ConfigScreenViewBase::handleTickEvent();

    // Só faz sentido “contar tempo” quando o teclado está visível
    if (teclado1Container.isVisible())
    {
    	teclado1Container.tick();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////




void ConfigScreenView::updateBarraSuperior(const TDataBarraSuperior& d)
{
	barraConteiner1.setData(d);
}

void ConfigScreenView::tearDownScreen()
{
    ConfigScreenViewBase::tearDownScreen();
}
