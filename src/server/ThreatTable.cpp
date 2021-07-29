#include "ThreatTable.h"

#include "Server.h"

void ThreatTable::makeAwareOf(Entity& entity) {
  auto it = _container.find(&entity);
  if (it == _container.end()) _container[&entity] = 0;
}

bool ThreatTable::isAwareOf(Entity& entity) const {
  auto it = _container.find(&entity);
  return (it != _container.end());
}

void ThreatTable::forgetAbout(const Entity& entity) {
  auto& nonConstRef = const_cast<Entity&>(entity);
  auto it = _container.find(&nonConstRef);
  if (it == _container.end()) return;
  _container.erase(it);
}

void ThreatTable::forgetCurrentTarget() {
  auto* target = getTarget();
  if (target) forgetAbout(*target);
}

void ThreatTable::addThreat(Entity& entity, Threat amount) {
  auto it = _container.find(&entity);
  if (it == _container.end())
    _container[&entity] = amount;
  else
    it->second += amount;
}

void ThreatTable::scaleThreat(Entity& entity, double multiplier) {
  auto it = _container.find(&entity);
  if (it == _container.end())
    return;
  else
    it->second = toInt(it->second * multiplier);
}

Entity* ThreatTable::getTarget() {
  auto highestThreat = -1;
  Entity* target = nullptr;

  for (auto pair : _container) {
    if (!pair.first->canBeAttackedBy(_owner)) continue;

    if (pair.second > highestThreat) {
      highestThreat = pair.second;
      target = pair.first;
    }
  }

  return target;
}

void ThreatTable::clear() { _container.clear(); }

bool ThreatTable::isEmpty() const { return _container.empty(); }

size_t ThreatTable::size() const { return _container.size(); }
