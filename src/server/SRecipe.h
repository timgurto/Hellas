#pragma once

#include "../Recipe.h"
#include "../types.h"

// Couples materials with products that players can craft.
class SRecipe : public Recipe {
  ms_t _time{0};
  bool _knownByDefault{false};

 public:
  SRecipe(const std::string &id) : Recipe(id) {}

  ms_t time() const { return _time; }
  void time(ms_t time) { _time = time; }
  void knownByDefault() { _knownByDefault = true; }
  bool isKnownByDefault() const { return _knownByDefault; }
};
