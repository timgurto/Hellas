#include "ObjectType.h"

ObjectType::ObjectType(const std::string &id):
_id(id),
_gatherTime(0),
_constructionTime(0),
_deconstructsItem(nullptr),
_deconstructionTime(0),
_gatherReq("none"),
_containerSlots(0),
_merchantSlots(0),
_collides(false){}

void ObjectType::addYield(const ServerItem *item, double initMean, double initSD, double gatherMean,
                          double gatherSD){
    _yield.addItem(item, initMean, initSD, gatherMean, gatherSD);
}

void ObjectType::addTag(const std::string &tagName){
    _tags.insert(tagName);
}

bool ObjectType::isTag( const std::string &tagName) const{
    return _tags.find(tagName) != _tags.end();
}
