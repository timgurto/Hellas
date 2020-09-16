#pragma once

#include "../server/User.h"

// This class acts as a size-one stack.  Use it to push altered User base stats.
// The original stats will be restored automatically when the object is
// destroyed.
class TemporaryUserStats {
 public:
  TemporaryUserStats();
  ~TemporaryUserStats();
  TemporaryUserStats &hps(Regen v);

 private:
  void apply();
  void storeCurrentSettings();
  void restoreOriginalSettings();
  Stats _new;
  Stats _old;
};

#define APPLY_TEMPORARY_USER_STATS       \
  TemporaryUserStats temporaryUserStats; \
  temporaryUserStats
