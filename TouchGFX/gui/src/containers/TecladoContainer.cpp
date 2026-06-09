#include <gui/containers/TecladoContainer.hpp>

static uint16_t min_u16(uint16_t a, uint16_t b) { return (a < b) ? a : b; }


TecladoContainer::TecladoContainer()
    : targetText(nullptr),
      targetBuffer(nullptr),
      targetBufferSize(0),
      btnCallback(this, &TecladoContainer::btnCallbackHandler),
      okCallback(nullptr),
	  firstKeyClears(true),
	  backupLen(0),
//	  tickCounter(0),
	  lastKeyTick(0),
	  minIntervalTicks(10)
{
	backupBuffer[0] = 0;
}

void TecladoContainer::initialize()
{
    TecladoContainerBase::initialize();

    // Liga todos os botões no mesmo handler
    button0.setAction(btnCallback);
    button1.setAction(btnCallback);
    button2.setAction(btnCallback);
    button3.setAction(btnCallback);
    button4.setAction(btnCallback);
    button5.setAction(btnCallback);
    button6.setAction(btnCallback);
    button7.setAction(btnCallback);
    button8.setAction(btnCallback);
    button9.setAction(btnCallback);

    buttonP.setAction(btnCallback);   // "."
    buttonB.setAction(btnCallback);   // backspace
    buttonOK.setAction(btnCallback);  // fecha
    buttonC.setAction(btnCallback);  // fecha
}

void TecladoContainer::setTarget(touchgfx::TextAreaWithOneWildcard* ta,
                                touchgfx::Unicode::UnicodeChar* buf,
                                uint16_t bufSize,
                                uint16_t maxCharsToType)
{
    targetText = ta;
    targetBuffer = buf;
    targetBufferSize = bufSize;

    maxChars = (maxCharsToType == 0) ? (bufSize - 1) : maxCharsToType;
    if (maxChars > (bufSize - 1)) maxChars = (bufSize - 1);

    // backup (Cancel)
    backupLen = 0;
    backupBuffer[0] = 0;

    if (targetBuffer && targetBufferSize > 0)
    {
        uint16_t n = min_u16((uint16_t)(BACKUP_MAX - 1), (uint16_t)(targetBufferSize - 1));
        for (uint16_t i = 0; i < n; i++)
        {
            backupBuffer[i] = targetBuffer[i];
            if (targetBuffer[i] == 0) { backupLen = i; break; }
            backupLen = i + 1;
        }
        backupBuffer[min_u16(backupLen, (uint16_t)(BACKUP_MAX - 1))] = 0;

        targetBuffer[targetBufferSize - 1] = 0;
    }

    firstKeyClears = true;
    if (targetText) targetText->invalidate();
}

void TecladoContainer::clearTarget()
{
    targetText = nullptr;
    targetBuffer = nullptr;
    targetBufferSize = 0;
    maxChars = 0;

    firstKeyClears = true;
    backupLen = 0;
    backupBuffer[0] = 0;
}

void TecladoContainer::setOkCallback(touchgfx::GenericCallback<const TecladoContainer&>* cb)
{
    okCallback = cb;
}

bool TecladoContainer::hasDot() const
{
    if (!targetBuffer) return false;
    uint16_t len = touchgfx::Unicode::strlen(targetBuffer);
    for (uint16_t i = 0; i < len; i++)
        if (targetBuffer[i] == (touchgfx::Unicode::UnicodeChar)'.') return true;
    return false;
}

void TecladoContainer::appendChar(touchgfx::Unicode::UnicodeChar c)
{
    if (!targetBuffer || targetBufferSize < 2) return;

    uint16_t len = touchgfx::Unicode::strlen(targetBuffer);
    if (len >= maxChars) return;

    if (c == '.' && hasDot()) return;

    targetBuffer[len] = c;
    targetBuffer[len + 1] = 0;

    if (targetText) targetText->invalidate();
}

void TecladoContainer::backspace()
{
    if (!targetBuffer) return;

    uint16_t len = touchgfx::Unicode::strlen(targetBuffer);
    if (len == 0) return;

    targetBuffer[len - 1] = 0;
    if (targetText) targetText->invalidate();
}

void TecladoContainer::clearText()
{
    if (!targetBuffer || targetBufferSize == 0) return;
    targetBuffer[0] = 0;
    if (targetText) targetText->invalidate();
}

void TecladoContainer::restoreBackup()
{
    if (!targetBuffer || targetBufferSize == 0) return;

    uint16_t n = min_u16((uint16_t)(targetBufferSize - 1), (uint16_t)(BACKUP_MAX - 1));
    for (uint16_t i = 0; i <= n; i++)
    {
        targetBuffer[i] = backupBuffer[i];
        if (backupBuffer[i] == 0) break;
    }
    targetBuffer[targetBufferSize - 1] = 0;
    if (targetText) targetText->invalidate();
}

void TecladoContainer::btnCallbackHandler(const touchgfx::AbstractButtonContainer& btn)
{
    // rate-limit
    if ((tickCounter - lastKeyTick) < minIntervalTicks) return;
    lastKeyTick = tickCounter;

    // CANCEL: restaura e fecha
    if (&btn == &buttonC)
    {
        restoreBackup();
        if (okCallback && okCallback->isValid()) okCallback->execute(*this);
        return;
    }

    // OK: fecha
    if (&btn == &buttonOK)
    {
        if (okCallback && okCallback->isValid()) okCallback->execute(*this);
        return;
    }

    // Backspace
    if (&btn == &buttonB)
    {
        if (firstKeyClears) { clearText(); firstKeyClears = false; }
        else { backspace(); }
        return;
    }

    // Ponto
    if (&btn == &buttonP)
    {
        if (firstKeyClears)
        {
            clearText();
            appendChar('0');
            appendChar('.');
            firstKeyClears = false;
        }
        else
        {
            appendChar('.');
        }
        return;
    }

    // Dígitos
    if (&btn == &button0 || &btn == &button1 || &btn == &button2 || &btn == &button3 ||
        &btn == &button4 || &btn == &button5 || &btn == &button6 || &btn == &button7 ||
        &btn == &button8 || &btn == &button9)
    {
        if (firstKeyClears) { clearText(); firstKeyClears = false; }

        if (&btn == &button0) appendChar('0');
        else if (&btn == &button1) appendChar('1');
        else if (&btn == &button2) appendChar('2');
        else if (&btn == &button3) appendChar('3');
        else if (&btn == &button4) appendChar('4');
        else if (&btn == &button5) appendChar('5');
        else if (&btn == &button6) appendChar('6');
        else if (&btn == &button7) appendChar('7');
        else if (&btn == &button8) appendChar('8');
        else if (&btn == &button9) appendChar('9');
    }
}


void TecladoContainer::tick()
{
    tickCounter++;
}

void TecladoContainer::setKeyRepeatTicks(uint16_t ticks)
{
    minIntervalTicks = ticks;
}

//void TecladoContainer::handleTickEvent()
//{
//    tickCounter++;
//}
