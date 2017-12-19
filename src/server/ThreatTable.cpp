#include "ThreatTable.h"

void ThreatTable::makeAwareOf(Entity & entity) {
    auto it = _container.find(&entity);
    if (it == _container.end())
        _container[&entity] = 0;
}

void ThreatTable::addThreat(Entity & entity, Threat amount) {
    auto it = _container.find(&entity);
    if (it == _container.end())
        _container[&entity] = amount;
    else
        it->second += amount;
}

Entity * ThreatTable::getTarget() {
    auto highestThreat = -1;
    Entity* target = nullptr;

    for (auto pair : _container)
        if (pair.second > highestThreat) {
            highestThreat = pair.second;
            target = pair.first;
        }

    return target;
}
