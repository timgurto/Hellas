#include "Entities.h"
#include "User.h"
#include "Vehicle.h"
#include "objects/Object.h"

Entity *Entities::find(size_t serial){
    Dummy dummy = Dummy::Serial(serial);
    auto it = _container.find(&dummy);
    if (it == _container.end())
        return nullptr;
    return *it;
}

const Vehicle * Entities::findVehicleDrivenBy(const User & driver) {
    for (const auto pEntity : _container) {
        auto vehicle = dynamic_cast<const Vehicle *>(pEntity);
        if (!vehicle)
            continue;
        if (vehicle->driver() == driver.name())
            return vehicle;
    }
    return nullptr;
}
