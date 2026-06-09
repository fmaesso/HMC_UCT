#ifndef BARRACONTEINER_HPP
#define BARRACONTEINER_HPP

#include <gui_generated/containers/barraConteinerBase.hpp>
#include "ui_data.h"


class barraConteiner : public barraConteinerBase
{
public:
    barraConteiner();
    virtual ~barraConteiner() {}

    virtual void initialize();

    void setData(const TDataBarraSuperior& d);

    void btnPlusClicked();
    void btnMinusClicked();
protected:
    TDataBarraSuperior  last;
	bool first = true;

	void updateTime(const char* s);
	void updateBattery(uint8_t pct);
//	void updatePower(TPwrState p);
//	void updateAlarm(TAlarmState a);

	touchgfx::Unicode::UnicodeChar clockTextBuffer[9];
	touchgfx::Unicode::UnicodeChar batTextBuffer[5]; // "100%"
};

#endif // BARRACONTEINER_HPP
