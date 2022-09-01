#pragma once

#include <mutex>
#include <set>
#include "..\types.h"

class Client;
class HasTags;

class CurrentTools {
 public:
  CurrentTools(Client& client) : _client(client) {}
  using Tools = std::set<std::string>;
  bool hasTool(std::string tag) const;
  bool hasAnyTools() const;
  const Tools& getTools() const { return _tools; }

  void update(ms_t timeElapsed);

  // Assumption: this is used in only one place (when refreshing the tools UI)
  bool hasChanged() const;  // [since last time this was called]

 private:
  void lookForTools();
  void includeItems();
  void includeObjects();
  void includeTerrain();
  void includeAllTagsFrom(const HasTags& thingWithTags);

  static const ms_t UPDATE_TIME{200};
  ms_t _timeSinceLastUpdate{0};
  Client& _client;

  mutable bool _hasChanged = true;

  mutable std::mutex _toolsMutex;
  Tools _tools;
};
