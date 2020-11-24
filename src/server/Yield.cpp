#include "Yield.h"

#include "../util.h"
#include "Server.h"
#include "User.h"

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

void Yield::loadFromXML(XmlReader xr, TiXmlElement *elem) {
  for (auto yield : xr.getChildren("yield", elem)) {
    auto id = ""s;
    if (!xr.findAttr(yield, "id", id)) continue;

    double initMean = 1., initSD = 0, gatherMean = 1, gatherSD = 0;
    size_t initMin = 0;
    xr.findAttr(yield, "initialMean", initMean);
    xr.findAttr(yield, "initialSD", initSD);
    xr.findAttr(yield, "initialMin", initMin);
    xr.findAttr(yield, "gatherMean", gatherMean);
    xr.findAttr(yield, "gatherSD", gatherSD);

    auto item = Server::instance().createAndFindItem(id);
    addItem(item, initMean, initSD, initMin, gatherMean, gatherSD);
  }
}

void Yield::simulate(const User &recipient) const {
  using QuantityCounts = std::map<int, int>;
  auto quantitiesByItem = std::map<std::string, QuantityCounts>{};

  for (auto i = 0; i != 1000 * 1000; ++i) {
    auto items = ItemSet{};
    instantiate(items);
    for (const auto &pair : items) {
      const auto item = pair.first;
      const auto qty = pair.second;

      quantitiesByItem[item->id()][qty]++;
    }
  }

  for (auto itemResults : quantitiesByItem) {
    auto message = itemResults.first + ": "s;
    for (auto pair : itemResults.second) {
      const auto qty = pair.first;
      const auto instancesOfThisQty = pair.second;
      message +=
          " " + toString(qty) + " ("s + toString(instancesOfThisQty) + ")"s;
    }
    recipient.sendMessage({SV_SYSTEM_MESSAGE, message});
  }
}

double Yield::gatherMean(const ServerItem *item) const {
  auto it = _entries.find(item);
  if (it == _entries.end()) {
    SERVER_ERROR("Object doesn't yield that item");
    return 0;
  }
  return it->second._gatherMean;
}
