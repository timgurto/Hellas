#include "Entities.h"
#include "Vehicle.h"
#include "objects/Object.h"

Entity *Entities::find(size_t serial){
    Dummy dummy = Dummy::Serial(serial);
    auto it = _container.find(&dummy);
    if (it == _container.end())
        return nullptr;
    return *it;
}