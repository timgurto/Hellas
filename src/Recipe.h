#pragma once

#include <set>
#include <string>

#include "Server/ItemSet.h"
#include "types.h"

using namespace std::string_literals;

class Recipe {
  std::string _id;
  ItemSet _materials;
  std::set<std::string> _tools;  // Tools required for crafting
  const Item *_product{nullptr};
  size_t _quantity{1};  // Quantity produced

 public:
  Recipe(const std::string &id);

  bool operator<(const Recipe &rhs) const { return _id < rhs._id; }

  const std::string &id() const { return _id; }
  const ItemSet &materials() const { return _materials; }
  const std::set<std::string> &tools() const { return _tools; }
  const Item *product() const { return _product; }
  void product(const Item *item) { _product = item; }
  size_t quantity() const { return _quantity; }
  void quantity(size_t n) { _quantity = n; }

  void addMaterial(const Item *item, size_t qty);
  void addTool(const std::string &name);
};
