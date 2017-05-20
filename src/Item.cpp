#include "Item.h"

const size_t Item::WEAPON_SLOT = 6;

Item::Item(const std::string &id):
    _id(id),
    _strength(0)
{}

void Item::addTag(const std::string &tagName){
    _tags.insert(tagName);
}

bool Item::isTag(const std::string &tagName) const{
    return _tags.find(tagName) != _tags.end();
}

size_t Item::getRandomArmorSlot(){
    /*
    Randomly return one of:
    0: head
    1: body
    3: shoulders
    4: hands
    5: feet
    7: left hand
    */
    size_t slot = rand() % 6;
    if (slot == 5)
        return 7;
    if (slot >= 1)
        return slot + 1;
    return slot;
}
