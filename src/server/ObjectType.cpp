#include <cassert>
#include "ObjectType.h"

ObjectType::ObjectType(const std::string &id):
_id(id),
_numInWorld(0),
_gatherTime(0),
_constructionTime(1000),
_deconstructsItem(nullptr),
_deconstructionTime(0),
_gatherReq("none"),
_isUnique(false),
_isUnbuildable(false),
_knownByDefault(false),
_merchantSlots(0),
_bottomlessMerchant(false),
_collides(false),
_allowedTerrain(nullptr),
_transformObject(nullptr),
_transformOnEmpty(false),
_container(nullptr)
{}

void ObjectType::addYield(const ServerItem *item,
                          double initMean, double initSD, size_t initMin,
                          double gatherMean, double gatherSD){
    _yield.addItem(item, initMean, initSD, initMin, gatherMean, gatherSD);
}

void ObjectType::addTag(const std::string &tagName){
    _tags.insert(tagName);
}

bool ObjectType::isTag( const std::string &tagName) const{
    return _tags.find(tagName) != _tags.end();
}

const TerrainList &ObjectType::allowedTerrain() const{
    if (_allowedTerrain == nullptr)
        return TerrainList::defaultList();
    else
        return *_allowedTerrain;
}

void ObjectType::checkUniquenessInvariant() const{
    if (_isUnique)
    assert (_numInWorld <= 1);
}
