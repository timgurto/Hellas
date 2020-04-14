#pragma once

#include "../Recipe.h"

class CRecipe : public Recipe {
 public:
  CRecipe(const std::string &id) : Recipe(id) {}

  void name(const std::string &recipeName) { _name = recipeName; }
  const std::string &name() const { return _name; }

  bool operator<(const CRecipe &rhs) const;

 private:
  std::string _name{};  // Default: product name.
};
