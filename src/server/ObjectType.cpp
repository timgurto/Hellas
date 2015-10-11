// (C) 2015 Tim Gurto

#include "ObjectType.h"

ObjectType::ObjectType(const std::string &id):
_id(id),
_actionTime(0),
_constructionTime(0),
_gatherReq("none"){}

void ObjectType::addYield(const Item *item, double initMean, double initSD, double gatherMean,
                          double gatherSD){
    _yield.addItem(item, initMean, initSD, gatherMean, gatherSD);
}
