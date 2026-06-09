#ifndef LOWRPM_HPP
#define LOWRPM_HPP

#include <gui_generated/containers/lowRPMBase.hpp>

class lowRPM : public lowRPMBase
{
public:
    lowRPM();
    virtual ~lowRPM() {}

    virtual void initialize();
protected:
    touchgfx::Callback<lowRPM, const touchgfx::AbstractButton&> lowRPMBtnCb;
    void onlowRPMButton(const touchgfx::AbstractButton& btn);
};

#endif // LOWRPM_HPP
