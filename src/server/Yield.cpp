#include <cassert>

#include "../util.h"
#include "Yield.h"

void Yield::addItem(const ServerItem *item, double initMean, double initSD,
                    size_t initMin, double gatherMean, double gatherSD) {
  _entries[item];
  YieldEntry *entry = &_entries[item];
  entry->_initDistribution = NormalVariable(initMean, initSD);
  entry->_gatherDistribution = NormalVariable(gatherMean, gatherSD);
  entry->_gatherMean = gatherMean;
  entry->_initMin = initMin;
}

void Yield::instantiate(ItemSet &contents) const {
  for (auto entry : _entries) {
    contents.set(entry.first, generateInitialQuantity(entry.second));
  }
}

size_t Yield::generateInitialQuantity(const YieldEntry &entry) {
  const double raw = entry._initDistribution();
  const double withMinimum = max<double>(entry._initMin, raw);
  return toInt(withMinimum);
}

size_t Yield::generateGatherQuantity(const ServerItem *item) const {
  const YieldEntry &entry = _entries.find(item)->second;
  double d = entry._gatherDistribution();
  return max<size_t>(
      1, toInt(d));  // User always gets at least one item when gathering
}

double Yield::gatherMean(const ServerItem *item) const {
  auto it = _entries.find(item);
  assert(it != _entries.end());
  return it->second._gatherMean;
}
