#include "Terrain.h"

Terrain *Terrain::empty() { return new Terrain; }

Terrain *Terrain::withTag(const std::string &tag) {
  Terrain *t = new Terrain;
  t->_tag = tag;
  return t;
}
