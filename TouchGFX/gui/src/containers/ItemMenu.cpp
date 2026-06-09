#include <gui/containers/ItemMenu.hpp>

ItemMenu::ItemMenu()
{

}

void ItemMenu::initialize()
{
    ItemMenuBase::initialize();
//    setTouchable(true);
}


void ItemMenu::setTitle(const char* t)
{
    // char* (UTF-8/ASCII) -> UnicodeChar
    touchgfx::Unicode::fromUTF8((const uint8_t*)t, ItemNameBuffer, ITEMNAME_SIZE);
    ItemName.invalidate();
}


