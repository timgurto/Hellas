#pragma once

#include <set>
#include "..\types.h"

class Client;

class CurrentTools {
 public:
  CurrentTools(const Client& client) : _client(client) {}
  using Tools = std::set<std::string>;
  bool hasTool(std::string tag) const;
  bool hasAnyTools() const;

  void update(ms_t timeElapsed);

 private:
  static const ms_t UPDATE_TIME{200};
  ms_t _timeUntilNextUpdate{0};
  const Client& _client;

  Tools _tools;
};
