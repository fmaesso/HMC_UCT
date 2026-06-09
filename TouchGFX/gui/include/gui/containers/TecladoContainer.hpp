#ifndef TECLADOCONTAINER_HPP
#define TECLADOCONTAINER_HPP

#include <gui_generated/containers/TecladoContainerBase.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/widgets/AbstractButton.hpp>

class TecladoContainer : public TecladoContainerBase
{
public:
    TecladoContainer();
    void initialize() override;
    // Define qual campo o teclado está editando
    void setTarget(touchgfx::TextAreaWithOneWildcard* ta,
                   touchgfx::Unicode::UnicodeChar* buf,
                   uint16_t bufSize, uint16_t maxCharsToType);

    // opcional: limpar target ao fechar
    void clearTarget();
//    void handleTickEvent() override;
    // opcional: callback para avisar a tela que apertou OK (fechar)
    void setOkCallback(touchgfx::GenericCallback<const TecladoContainer&>* cb);

    void tick();                // <-- novo
    void setKeyRepeatTicks(uint16_t ticks);  // opcional
protected:


private:
    // Target atual
    touchgfx::TextAreaWithOneWildcard* targetText;
    touchgfx::Unicode::UnicodeChar* targetBuffer;
    uint16_t targetBufferSize;
    uint16_t maxChars;

    // 1ª tecla limpa / Cancel restaura
	bool firstKeyClears;
	static const uint16_t BACKUP_MAX = 32;
	touchgfx::Unicode::UnicodeChar backupBuffer[BACKUP_MAX];
	uint16_t backupLen;

    uint32_t tickCounter;
    uint32_t lastKeyTick;
    uint32_t minIntervalTicks;

    // Callbacks dos botões
//    touchgfx::Callback<TecladoContainer, const touchgfx::AbstractButton&> btnCallback;
//    void btnCallbackHandler(const touchgfx::AbstractButton& btn);

    touchgfx::Callback<TecladoContainer, const touchgfx::AbstractButtonContainer&> btnCallback;
    void btnCallbackHandler(const touchgfx::AbstractButtonContainer& btn);

    touchgfx::GenericCallback<const TecladoContainer&>* okCallback;

    // Helpers
    void appendChar(touchgfx::Unicode::UnicodeChar c);
    void backspace();
    bool hasDot() const;

    void clearText();
    void restoreBackup();
};

#endif // TECLADOCONTAINER_HPP
