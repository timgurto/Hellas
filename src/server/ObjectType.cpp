// (C) 2015 Tim Gurto

#include "ObjectType.h"

ObjectType::ObjectType(const std::string &id):
_id(id),
_gatherTime(0),
_constructionTime(0),
_deconstructsItem(0),
_deconstructionTime(0),
_gatherReq("none"),
_containerSlots(0),
_merchantSlots(0),
_collides(false){}

ObjectType::ObjectType(const Rect &collisionRect):
_gatherTime(0),
_constructionTime(0),
_deconstructsItem(0),
_deconstructionTime(0),
_gatherReq("none"),
_containerSlots(0),
_merchantSlots(0),
_collides(true),
_collisionRect(collisionRect){}

void ObjectType::addYield(const Item *item, double initMean, double initSD, double gatherMean,
                          double gatherSD){
    _yield.addItem(item, initMean, initSD, gatherMean, gatherSD);
}

void ObjectType::addClass(const std::string &className){
    _classes.insert(className);
}

bool ObjectType::isClass( const std::string &className) const{
    return _classes.find(className) != _classes.end();
}
