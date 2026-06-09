#ifndef SAVEDCONTAINER_HPP
#define SAVEDCONTAINER_HPP

#include <gui_generated/containers/savedContainerBase.hpp>

class savedContainer : public savedContainerBase
{
public:
    savedContainer();
    virtual ~savedContainer() {}

    virtual void initialize();
protected:
};

#endif // SAVEDCONTAINER_HPP
