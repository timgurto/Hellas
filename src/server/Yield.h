#ifndef YIELD_H
#define YIELD_H

#include <map>
#include <string>

#include "../NormalVariable.h"
#include "ItemSet.h"
#include "ServerItem.h"

// Provides a way for objects to give players items when gathered.  This is part
// of an EntityType.
class Yield {
  struct YieldEntry {
    NormalVariable _initDistribution, _gatherDistribution;
    size_t _initMin;
    double _gatherMean;
  };

  std::map<const ServerItem *, YieldEntry> _entries;

 public:
  operator bool() const { return !_entries.empty(); }

  double gatherMean(const ServerItem *item) const;

  void addItem(const ServerItem *item, double initMean = 1, double initSD = 0,
               size_t initMin = 0, double gatherMean = 1., double gatherSD = 0);

  // Creates a new instance of this Yield, with random init values, in the
  // specified ItemSet
  void instantiate(ItemSet &contents) const;

  // Generate a normally-distributed random number based on the mean and SD of
  // an entry
  static size_t generateInitialQuantity(const YieldEntry &entry);
  size_t generateGatherQuantity(const ServerItem *item) const;

  void loadFromXML(XmlReader xr, TiXmlElement *elem);

  void requiresTool(const std::string &tool) { _requiredTool = tool; }
  const std::string &requiredTool() const { return _requiredTool; }
  void gatherTime(ms_t t) { _gatherTime = t; }
  ms_t gatherTime() const { return _gatherTime; }

  void simulate(const User &recipient) const;

 private:
  std::string _requiredTool;
  ms_t _gatherTime{0};
};

#endif
