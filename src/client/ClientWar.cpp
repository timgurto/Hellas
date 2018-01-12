#include "ClientWar.h"

void YourWars::add(const std::string & name) {
    auto it = _container.find(name);
    if (it == _container.end())
        _container[name] = NO_PEACE_PROPOSED;
}

bool YourWars::atWarWith(const std::string & name) const {
    return _container.find(name) != _container.end();
}
