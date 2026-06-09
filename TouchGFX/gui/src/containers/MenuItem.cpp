#include <gui/containers/MenuItem.hpp>
#include <touchgfx/Unicode.hpp>
#include <cstdlib>

MenuItem::MenuItem()
    : btnCb(this, &MenuItem::onButtonPressed)
{
}

void MenuItem::initialize()
{
    MenuItemBase::initialize();

    // Texto com wildcard
    textMenu.setWildcard(textMenuBuffer);

    // Botão invisível que cobre o item e captura toque
    flexButton.setVisible(true);
    flexButton.setTouchable(true);
    flexButton.setAlpha(60);

    // Trigger está em TOUCH (press), então o action dispara no toque
    flexButton.setAction(btnCb);

    // Container também tocável (ajuda em alguns casos)
    setTouchable(true);

    // Zera estado
    moved = false;
    startX = startY = 0;
}

void MenuItem::setTitle(const char* t)
{
    // char* (UTF-8/ASCII) -> UnicodeChar
    touchgfx::Unicode::fromUTF8((const uint8_t*)t, textMenuBuffer, TEXTMENU_SIZE);
    textMenu.invalidate();
}

// Esse callback vai disparar NO PRESS (trigger Touch)
// Aqui a gente NÃO executa ainda se quiser filtrar por drag.
// Como o trigger é no press, o filtro perfeito seria executar no RELEASE,
// mas o ScrollList pode roubar RELEASE. Então a estratégia é:
// - executar no press, mas só se ainda não moveu.
// - e zerar moved no PRESS.
void MenuItem::onButtonPressed(const touchgfx::AbstractButtonContainer& src)
{
    (void)src;

    // Se já detectamos que virou drag, não executa
    if (!moved)
    {
        if (action) action->execute(index);
    }
}

// Se estes eventos chegarem, melhor ainda: filtramos corretamente
void MenuItem::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    MenuItemBase::handleClickEvent(evt);

    if (evt.getType() == touchgfx::ClickEvent::PRESSED)
    {
        startX = evt.getX();
        startY = evt.getY();
        moved = false;
    }
}

void MenuItem::handleDragEvent(const touchgfx::DragEvent& evt)
{
    MenuItemBase::handleDragEvent(evt);

    // diferença absoluta desde o início
    const int16_t dx = evt.getNewX() - startX;
    const int16_t dy = evt.getNewY() - startY;

    if (!moved)
    {
        if (std::abs(dx) > DRAG_THRESHOLD || std::abs(dy) > DRAG_THRESHOLD)
            moved = true;
    }
}
