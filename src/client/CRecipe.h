#pragma once

#include "../Recipe.h"
#include "HasSounds.h"
#include "Tag.h"
#include "Tooltip.h"

class CRecipe : public Recipe, public HasSounds {
 public:
  CRecipe(const std::string &id) : Recipe(id) {}

  void name(const std::string &recipeName) { _name = recipeName; }
  const std::string &name() const { return _name; }
  void category(const std::string &cat) { _category = cat; }
  const std::string &category() const { return _category; }
  const Tooltip &tooltip(const TagNames &tagnames) const;

 private:
  std::string _name{};  // Default: product name.
  std::string _category{};
  mutable Tooltip _tooltip;
};
