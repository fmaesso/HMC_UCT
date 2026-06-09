#ifndef DATAHORACONTAINER_HPP
#define DATAHORACONTAINER_HPP

#include <gui_generated/containers/DataHoraContainerBase.hpp>

class DataHoraContainer : public DataHoraContainerBase
{
public:
    DataHoraContainer();
    virtual ~DataHoraContainer() {}
    virtual void initialize()
        {
            DataHoraContainerBase::initialize();
            // Em geral o Base já liga o wildcard, mas manter aqui não atrapalha
            valDH.invalidate();
        }

        // Mostra string ASCII/UTF-8 curta (ex: "12:34" ou "Item 1")
        void setText(const char* str);

        // Mostra 2 dígitos "00".."99"
        void set2digits(uint8_t v);

        // Mostra "HH:MM"
        void setTimeHM(uint8_t hh, uint8_t mm);
protected:
};

#endif // DATAHORACONTAINER_HPP
