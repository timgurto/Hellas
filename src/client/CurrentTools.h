#pragma once

#include <mutex>
#include <set>
#include "..\types.h"

class Client;
class HasTags;

class CurrentTools {
 public:
  CurrentTools(const Client& client) : _client(client) {}
  using Tools = std::set<std::string>;
  bool hasTool(std::string tag) const;
  bool hasAnyTools() const;

  void update(ms_t timeElapsed);

 private:
  void lookForTools();
  void includeItems();
  void includeObjects();
  void includeTerrain();
  void includeTags(const HasTags& thingWithTags);

  static const ms_t UPDATE_TIME{200};
  ms_t _timeUntilNextUpdate{0};
  const Client& _client;

  mutable std::mutex _toolsMutex;
  Tools _tools;
};
