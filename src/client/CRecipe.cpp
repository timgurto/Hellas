#include "CRecipe.h"

bool CRecipe::operator<(const CRecipe& rhs) const {
  if (_name != rhs._name) return _name < rhs._name;
  return id() < rhs.id();
}
