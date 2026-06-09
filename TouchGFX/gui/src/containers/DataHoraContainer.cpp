#include <gui/containers/DataHoraContainer.hpp>

DataHoraContainer::DataHoraContainer()
{

}

//void DataHoraContainer::initialize()
//{
//    DataHoraContainerBase::initialize();
//}

void DataHoraContainer::setText(const char* str)
{
    touchgfx::Unicode::fromUTF8((const uint8_t*)str, valDHBuffer, VALDH_SIZE);
//    valDH.setWildcard(valDHBuffer);
    valDH.invalidate();
}

void DataHoraContainer::set2digits(uint8_t v)
{
    touchgfx::Unicode::snprintf(valDHBuffer, VALDH_SIZE, "%02d", v);
//    valDH.setWildcard(valDHBuffer);
    valDH.invalidate();
}

void DataHoraContainer::setTimeHM(uint8_t hh, uint8_t mm)
{
    touchgfx::Unicode::snprintf(valDHBuffer, VALDH_SIZE, "%02d:%02d", hh, mm);
    valDH.invalidate();
}
