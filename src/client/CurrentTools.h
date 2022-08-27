#pragma once

#include <set>
#include "..\HasTags.h"
#include "..\types.h"

class Client;

class CurrentTools : public HasTags {
 public:
  CurrentTools(const Client& client) : _client(client) {}

  void update(ms_t timeElapsed);

 private:
  static const ms_t UPDATE_TIME{200};
  ms_t _timeUntilNextUpdate{0};
  const Client& _client;
};
