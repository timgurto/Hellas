#include <cassert>

#include "ClientWar.h"

void YourWars::add(const std::string & name) {
    auto it = _container.find(name);
    if (it == _container.end())
        _container[name] = NO_PEACE_PROPOSED;
}

bool YourWars::atWarWith(const std::string & name) const {
    return _container.find(name) != _container.end();
}

void YourWars::proposePeaceWith(const std::string & name) {
    auto it = _container.find(name);
    assert(it != _container.end());
    it->second = PEACE_PROPOSED_BY_YOU;
}
