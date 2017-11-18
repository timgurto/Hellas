#include "Class.h"

Class::Class(const ClassType *type) :
    _type(type)
{}

std::string Class::generateKnownSpellsString() const {
    auto string = ""s;
    for (const auto &spell : _learnedSpells) {
        if (!string.empty())
            string.append(",");
        string.append(spell);
    }
    return string;
}
