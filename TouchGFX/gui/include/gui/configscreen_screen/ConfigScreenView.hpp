#ifndef CONFIGSCREENVIEW_HPP
#define CONFIGSCREENVIEW_HPP

#include <gui_generated/configscreen_screen/ConfigScreenViewBase.hpp>
#include <gui/configscreen_screen/ConfigScreenPresenter.hpp>
#include "ui_data.h"
#include "cec.h"

class ConfigScreenView : public ConfigScreenViewBase
{
public:
    ConfigScreenView();
    virtual ~ConfigScreenView() {}
    virtual void setupScreen();
    virtual void updateBarraSuperior(const TDataBarraSuperior& d);
    virtual void getValoresConfig();
    virtual void scrollList2UpdateItem(ItemMenu& item, int16_t itemIndex);//{// Override and implement this function in BaseConfig    }


    virtual void wheelHourUpdateItem(DataHoraContainer& item, int16_t itemIndex);
    virtual void wheelMinUpdateItem(DataHoraContainer& item, int16_t itemIndex);
    virtual void wheelMonthUpdateItem(DataHoraContainer& item, int16_t itemIndex);
    virtual void wheelDayUpdateItem(DataHoraContainer& item, int16_t itemIndex);
    virtual void wheelYearUpdateItem(DataHoraContainer& item, int16_t itemIndex);

    void handleTickEvent() override;

    virtual void tearDownScreen();
protected:
//referente ao scrollList do menu principal
    static const uint16_t N2 = 8;
    ItemMenu itemM[N2];

    touchgfx::Callback<ConfigScreenView, int16_t> scrollList2ItemSelectedCallback;
    void scrollList2ItemSelectedHandler(int16_t itemSelected);
    void onMenuClicked(uint16_t idx);
//referente aos scrollwheel para data hora
    void SetaWheelClock();
    void SetaTeclado();
    void BotoesEdicao();
    // Callback do botão aplicar

	touchgfx::Callback<ConfigScreenView, const touchgfx::AbstractButton&> applyBtnCb;
	void onApplyButton(const touchgfx::AbstractButton& btn);

	touchgfx::Callback<ConfigScreenView, const touchgfx::AbstractButton&> saveBtnCb;
	void onSaveButton(const touchgfx::AbstractButton& btn);

    uint8_t selHour = 0;
    uint8_t selMin  = 0;
    uint8_t selDia  = 0;
    uint8_t selMes  = 0;
    uint8_t selAno  = 0;

    void updateMenuSelectTextItem(int16_t itemIndex);

//referente ao teclado

    touchgfx::Callback<ConfigScreenView, const TecladoContainer&> tecladoOkCb;
	void tecladoOkHandler(const TecladoContainer&);

	touchgfx::Callback<ConfigScreenView, const TecladoContainer&> tecladoCCb;
	void tecladoCancelHandler(const TecladoContainer&);

	// callbacks dos botões transparentes (cada um com seu handler)
	touchgfx::Callback<ConfigScreenView, const touchgfx::AbstractButtonContainer&> fbCb;
	void fbHandler(const touchgfx::AbstractButtonContainer& src);

	void openKeyboardFor(touchgfx::TextAreaWithOneWildcard& ta,
						 touchgfx::Unicode::UnicodeChar* buf,
						 uint16_t bufSize,
						 int16_t x, int16_t y, int16_t max);

	touchgfx::TextAreaWithOneWildcard* activeText;
	touchgfx::colortype normalColor;
	touchgfx::colortype editColor;
	void commitEditingIfNeeded();
	void closeKeyboardUiOnly();
	configs_t cp_cfg;
	void SetValoresConfig();

};

#endif // CONFIGSCREENVIEW_HPP
