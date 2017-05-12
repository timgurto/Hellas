#ifndef OBJECT_TYPE_H
#define OBJECT_TYPE_H

#include <string>

#include "Container.h"
#include "Deconstruction.h"
#include "TerrainList.h"
#include "Yield.h"
#include "../Rect.h"
#include "../types.h"

class ServerItem;

// Describes a class of Objects, the "instances" of which share common properties
class ObjectType{
    std::string _id;

    mutable size_t _numInWorld;

    std::string _constructionReq;
    ms_t _constructionTime;
    bool _isUnique; // Can only exist once at a time in the world.
    bool _isUnbuildable; // Data suggests it can be built, but direct construction should be blocked.

    // To gather from objects of this type, a user must have an item of the following class.
    std::string _gatherReq;
    ms_t _gatherTime;

    size_t _merchantSlots;
    bool _bottomlessMerchant; // Bottomless: never runs out, uses no inventory space.

    Yield _yield; // If gatherable.

    bool _collides; // false by default; true if any collisionRect is specified.
    Rect _collisionRect; // Relative to position

    const ObjectType *_transformObject; // The object type that this becomes over time, if any.
    ms_t _transformTime; // How long the transformation takes.
    bool _transformOnEmpty; // Only begin the transformation once all items have been gathered.

    std::set<std::string> _tags;

    const TerrainList *_allowedTerrain;

    ItemSet _materials; // The necessary materials, if this needs to be constructed in-place.
    bool _knownByDefault;


protected:
    ContainerType *_container;
    DeconstructionType *_deconstruction;

public:
    ObjectType(const std::string &id);

    virtual ~ObjectType(){}

    void gatherTime(ms_t t) { _gatherTime = t; }
    const std::string &gatherReq() const { return _gatherReq; }
    void gatherReq(const std::string &req) { _gatherReq = req; }
    const std::string &constructionReq() const { return _constructionReq; }
    void constructionReq(const std::string &req) { _constructionReq = req; }
    size_t merchantSlots() const { return _merchantSlots; }
    void merchantSlots(size_t n) { _merchantSlots = n; }
    bool bottomlessMerchant() const { return _bottomlessMerchant; }
    void bottomlessMerchant(bool b) { _bottomlessMerchant = b; }
    void knownByDefault() { _knownByDefault = true; }
    bool isKnownByDefault() const { return _knownByDefault; }
    void makeUnique() { _isUnique = true; checkUniquenessInvariant(); }
    bool isUnique() const { return _isUnique; }
    void makeUnbuildable() { _isUnbuildable = true; }
    bool isUnbuildable() const { return _isUnbuildable; }
    void incrementCounter() const { ++ _numInWorld; checkUniquenessInvariant(); }
    void decrementCounter() const { -- _numInWorld; }
    size_t numInWorld() const { return _numInWorld; }

    virtual char classTag() const { return 'o'; }

    const std::string &id() const { return _id; }
    ms_t gatherTime() const { return _gatherTime; }
    ms_t constructionTime() const { return _constructionTime; }
    void constructionTime(ms_t t) { _constructionTime = t; }
    const Yield &yield() const { return _yield; }
    bool collides() const { return _collides; }
    const Rect &collisionRect() const { return _collisionRect; }
    void collisionRect(const Rect &r) { _collisionRect = r; _collides = true; }
    bool isTag( const std::string &tagName) const;
    const TerrainList &allowedTerrain() const;
    void allowedTerrain(const std::string &id){ _allowedTerrain = TerrainList::findList(id); }
    void addMaterial(const Item *material, size_t quantity) { _materials.add(material, quantity); }
    const ItemSet &materials() const { return _materials; }
    void transform(ms_t time, const ObjectType *id) {_transformTime = time; _transformObject = id;}
    ms_t transformTime() const {return _transformTime; }
    void transformOnEmpty() { _transformOnEmpty = true; }
    bool transformsOnEmpty() const { return _transformOnEmpty; }
    const ObjectType *transformObject() const {return _transformObject; }
    bool transforms() const { return _transformObject != nullptr; }

    void checkUniquenessInvariant() const;

    bool operator<(const ObjectType &rhs) const { return _id < rhs._id; }

    void addYield(const ServerItem *item,
                  double initMean, double initSD, size_t initMin,
                  double gatherMean, double gatherSD);
    void addTag(const std::string &tagName);

    
    bool hasContainer() const { return _container != nullptr; }
    ContainerType &container() { return *_container; }
    const ContainerType &container() const { return *_container; }
    void addContainer(ContainerType *p) { _container = p; }

    bool hasDeconstruction() const { return _deconstruction != nullptr; }
    DeconstructionType &deconstruction() { return *_deconstruction; }
    const DeconstructionType &deconstruction() const { return *_deconstruction; }
    void addDeconstruction(DeconstructionType *p) { _deconstruction = p; }
};

#endif
