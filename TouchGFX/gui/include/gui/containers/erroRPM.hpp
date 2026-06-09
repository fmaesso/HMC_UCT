#ifndef ERRORPM_HPP
#define ERRORPM_HPP

#include <gui_generated/containers/erroRPMBase.hpp>

class erroRPM : public erroRPMBase
{
public:
    erroRPM();
    virtual ~erroRPM() {}

    virtual void initialize();
protected:
    touchgfx::Callback<erroRPM, const touchgfx::AbstractButton&> erroRPMBtnCb;
	void onErroRPMButton(const touchgfx::AbstractButton& btn);
};

#endif // ERRORPM_HPP
