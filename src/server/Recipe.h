#ifndef RECIPE_H
#define RECIPE_H

#include <SDL.h>
#include <set>
#include <string>

#include "../types.h"
#include "ItemSet.h"

using namespace std::string_literals;

// Couples materials with products that players can craft.
class Recipe {
  std::string _id;
  ItemSet _materials;
  std::set<std::string> _tools;  // Tools required for crafting
  const Item *_product;
  size_t _quantity;  // Quantity produced
  ms_t _time;
  bool _knownByDefault;
  std::string _name = ""s;  // Default: product name.  Not used on server.

 public:
  Recipe(const std::string &id);  // time = 0, ptrs = nullptr

  bool operator<(const Recipe &rhs) const { return _id < rhs._id; }

  const std::string &id() const { return _id; }
  const ItemSet &materials() const { return _materials; }
  const std::set<std::string> &tools() const { return _tools; }
  const Item *product() const { return _product; }
  void product(const Item *item) { _product = item; }
  size_t quantity() const { return _quantity; }
  void quantity(size_t n) { _quantity = n; }
  ms_t time() const { return _time; }
  void time(ms_t time) { _time = time; }
  void knownByDefault() { _knownByDefault = true; }
  bool isKnownByDefault() const { return _knownByDefault; }
  void name(const std::string &recipeName) { _name = recipeName; }
  const std::string &name() const { return _name; }

  void addMaterial(const Item *item, size_t qty);
  void addTool(const std::string &name);
};

#endif
