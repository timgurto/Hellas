#include "Edge.h"

Edge::Edge(const std::string &from, const std::string &to, EdgeType typeArg, double chanceArg):
    parent(from),
    child(to),
    type(typeArg),
    chance(chanceArg)
    {}

bool Edge::operator==(const Edge &rhs) const {
    return parent == rhs.parent && child == rhs.child;
}
   
bool Edge::operator<(const Edge &rhs) const{
    if (parent != rhs.parent) return parent < rhs.parent;
    return child < rhs.child;
}

std::ostream &operator<<(std::ostream &lhs, const Edge &rhs){
    lhs << rhs.parent << " -> " << rhs.child;
    return lhs;
}
