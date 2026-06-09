#ifndef MENUITEM_HPP
#define MENUITEM_HPP

#include <gui_generated/containers/MenuItemBase.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/events/DragEvent.hpp>

class MenuItem : public MenuItemBase
{
public:
    MenuItem();
    virtual ~MenuItem() {}

    virtual void initialize();

    void setIndex(uint16_t i) { index = i; }
    void setTitle(const char* t);

    // View passa o callback aqui
    void setAction(touchgfx::GenericCallback<uint16_t>* cb) { action = cb; }

private:
    uint16_t index = 0;
    touchgfx::GenericCallback<uint16_t>* action = nullptr;

    // filtro scroll vs click
    int16_t startX = 0;
    int16_t startY = 0;
    bool moved = false;
    static const int16_t DRAG_THRESHOLD = 8; // pixels

    // callback do flexButton1
    touchgfx::Callback<MenuItem, const touchgfx::AbstractButtonContainer&> btnCb;
    void onButtonPressed(const touchgfx::AbstractButtonContainer& src);

protected:
    // vamos monitorar gesto/drag no item (quando chegar)
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);
    virtual void handleDragEvent(const touchgfx::DragEvent& evt);
};

#endif // MENUITEM_HPP
