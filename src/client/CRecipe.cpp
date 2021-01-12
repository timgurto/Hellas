#include "CRecipe.h"

const Tooltip &CRecipe::tooltip(const TagNames &tagnames) const {
  if (_tooltip) return _tooltip;

  _tooltip = {};
  _tooltip.addRecipe(*this, tagnames);
  return _tooltip;
}
