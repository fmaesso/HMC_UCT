#ifndef ITEMMENU_HPP
#define ITEMMENU_HPP

#include <gui_generated/containers/ItemMenuBase.hpp>

class ItemMenu : public ItemMenuBase
{
public:
    ItemMenu();
    virtual ~ItemMenu() {}
    void setIndex(uint16_t i) { indexM = i; }
    void setTitle(const char* t);
    virtual void initialize();
protected:
    touchgfx::GenericCallback<uint16_t>* action = nullptr;
    uint16_t indexM = 0;

};

#endif // ITEMMENU_HPP
