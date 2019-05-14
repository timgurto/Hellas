#include "Unlocks.h"

Unlocks::Container Unlocks::_container;

bool operator<(const Unlocks::Trigger &lhs, const Unlocks::Trigger &rhs) {
  if (lhs.type != rhs.type) return lhs.type < rhs.type;
  return lhs.id < rhs.id;
}

bool operator<(const Unlocks::Effect &lhs, const Unlocks::Effect &rhs) {
  if (lhs.type != rhs.type) return lhs.type < rhs.type;
  return lhs.id < rhs.id;
}

void Unlocks::add(const Trigger &trigger, const Effect &effect, double chance) {
  _container[trigger][effect] = chance;
}

Unlocks::EffectInfo Unlocks::getEffectInfo(const Trigger &trigger) {
  auto it = _container.find(trigger);
  if (it == _container.end()) return {false, {}, {}};

  EffectInfo ret;
  ret.hasEffect = true;
  ret.color = Color::TOOLTIP_BODY;
  ret.message = "Might unlock something";
  return ret;
}
