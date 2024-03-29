#include "ClientWar.h"

void YourWars::add(const std::string &name) {
  auto it = _container.find(name);
  if (it == _container.end()) _container[name] = NO_PEACE_PROPOSED;
}

bool YourWars::atWarWith(const std::string &name) const {
  return _container.find(name) != _container.end();
}

void YourWars::proposePeaceWith(const std::string &name) {
  auto it = _container.find(name);
  if (it == _container.end()) return;
  it->second = PEACE_PROPOSED_BY_YOU;
}

void YourWars::peaceWasProposedBy(const std::string &name) {
  auto it = _container.find(name);
  if (it == _container.end()) return;
  it->second = PEACE_PROPOSED_BY_HIM;
}

void YourWars::cancelPeaceOffer(const std::string &name) {
  auto it = _container.find(name);
  if (it == _container.end()) return;
  it->second = NO_PEACE_PROPOSED;
}

void YourWars::remove(const std::string &name) {
  auto it = _container.find(name);
  if (it == _container.end()) return;
  _container.erase(it);
}
