#include "Class.h"

LearnedSpells && ClassType::instantiateLearnedSpellsList() const {
    auto newList = LearnedSpells{};
    return std::move(newList);
}

Class::Class(const ClassType &type) :
    _type(type),
    _learnedSpells(std::move(type.instantiateLearnedSpellsList())) {
}
