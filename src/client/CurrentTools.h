#pragma once

#include <set>
#include "..\types.h"

class Client;

class CurrentTools {
 public:
  CurrentTools(const Client& client) : _client(client) {}
  using Tools = std::set<std::string>;
  const Tools& tools() const { return _tools; }

  void update(ms_t timeElapsed);

 private:
  const Client& _client;
  Tools _tools;
};
