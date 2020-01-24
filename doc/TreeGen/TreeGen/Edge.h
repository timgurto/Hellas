#ifndef EDGE_H
#define EDGE_H

#include <string>

enum EdgeType {
  GATHER,
  CONSTRUCT_FROM_ITEM,
  GATHER_REQ,
  CONSTRUCTION_REQ,
  LOOT,
  INGREDIENT,
  TRANSFORM,

  UNLOCK_ON_GATHER,
  UNLOCK_ON_ACQUIRE,
  UNLOCK_ON_CRAFT,
  UNLOCK_ON_CONSTRUCT,

  DEFAULT
};

struct Edge {
  std::string parent, child;
  EdgeType type;
  double chance;

  Edge(const std::string &from, const std::string &to, EdgeType type,
       double chance = 1.0);
  bool operator==(const Edge &rhs) const;
  bool operator<(const Edge &rhs) const;
};
std::ostream &operator<<(std::ostream &lhs, const Edge &rhs);

#endif
