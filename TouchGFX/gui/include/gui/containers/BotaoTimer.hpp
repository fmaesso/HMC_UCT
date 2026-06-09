#ifndef BOTAOTIMER_HPP
#define BOTAOTIMER_HPP

#include <gui_generated/containers/BotaoTimerBase.hpp>

class BotaoTimer : public BotaoTimerBase
{
public:
    BotaoTimer();
    virtual ~BotaoTimer() {}

    virtual void initialize();
protected:
};

#endif // BOTAOTIMER_HPP
