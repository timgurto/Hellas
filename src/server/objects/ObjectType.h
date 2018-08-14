#ifndef OBJECT_TYPE_H
#define OBJECT_TYPE_H

#include "../../types.h"
#include "../EntityType.h"
#include "../QuestNode.h"
#include "../Yield.h"
#include "Action.h"
#include "Container.h"
#include "Deconstruction.h"

class ServerItem;

// Describes a class of Objects, the "instances" of which share common
// properties
class ObjectType : public EntityType, public QuestNodeType {
  class Strength {
   public:
    Strength();
    void set(const ServerItem *item, size_t quantity);
    Hitpoints get() const;
    const ServerItem *item() const { return _item; }
    size_t quantity() const { return _quantity; }

   private:
    const ServerItem *_item;
    size_t _quantity;
    mutable Hitpoints _calculatedStrength;
    mutable bool _strengthCalculated;
  };
  Strength _strength;

  mutable size_t _numInWorld;

  std::string _constructionReq;
  ms_t _constructionTime;
  bool _isUnique;       // Can only exist once at a time in the world.
  bool _isUnbuildable;  // Data suggests it can be built, but direct
                        // construction should be blocked.

  // To gather from objects of this type, a user must have an item of the
  // following class.
  std::string _gatherReq;
  ms_t _gatherTime;

  size_t _merchantSlots;
  bool _bottomlessMerchant;  // Bottomless: never runs out, uses no inventory
                             // space.

  Yield _yield;  // If gatherable.

  const ObjectType *_transformObject =
      nullptr;  // The object type that this becomes over time, if any.
  ms_t _transformTime = 0;         // How long the transformation takes.
  bool _transformOnEmpty = false;  // Begin the transformation only once all
                                   // items have been gathered.
  bool _skipConstructionOnTransform =
      false;  // If true, the new object will be fully constructed

  ms_t _disappearsAfter{
      0};  // After an object has existed for this long, it disappears.

  ItemSet _materials;  // The necessary materials, if this needs to be
                       // constructed in-place.
  bool _knownByDefault;

  std::string
      _playerUniqueCategory;  // Assumption: up to one category per object type.

 protected:
  ContainerType *_container;
  DeconstructionType *_deconstruction;

  Action *_action = nullptr;
  CallbackAction *_onDestroy = nullptr;

 public:
  ObjectType(const std::string &id);

  virtual ~ObjectType() {}

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
  void makeUnique() {
    _isUnique = true;
    checkUniquenessInvariant();
  }
  bool isUnique() const { return _isUnique; }
  void makeUnbuildable() { _isUnbuildable = true; }
  bool isUnbuildable() const { return _isUnbuildable; }
  void incrementCounter() const {
    ++_numInWorld;
    checkUniquenessInvariant();
  }
  void decrementCounter() const { --_numInWorld; }
  size_t numInWorld() const { return _numInWorld; }
  void makeUniquePerPlayer(const std::string &category) {
    _playerUniqueCategory = category;
  }
  bool isPlayerUnique() const { return !_playerUniqueCategory.empty(); }
  const std::string &playerUniqueCategory() const {
    return _playerUniqueCategory;
  }

  virtual char classTag() const override { return 'o'; }

  ms_t gatherTime() const { return _gatherTime; }
  ms_t constructionTime() const { return _constructionTime; }
  void constructionTime(ms_t t) { _constructionTime = t; }
  const Yield &yield() const { return _yield; }
  void addMaterial(const Item *material, size_t quantity) {
    _materials.add(material, quantity);
  }
  const ItemSet &materials() const { return _materials; }
  void transform(ms_t time, const ObjectType *id) {
    _transformTime = time;
    _transformObject = id;
  }
  ms_t transformTime() const { return _transformTime; }
  void transformOnEmpty() { _transformOnEmpty = true; }
  bool transformsOnEmpty() const { return _transformOnEmpty; }
  const ObjectType *transformObject() const { return _transformObject; }
  bool transforms() const { return _transformObject != nullptr; }
  void skipConstructionOnTransform(bool b) { _skipConstructionOnTransform = b; }
  bool skipConstructionOnTransform() const {
    return _skipConstructionOnTransform;
  }
  void disappearsAfter(ms_t time) { _disappearsAfter = time; }
  ms_t disappearsAfter() const { return _disappearsAfter; }
  void setHealthBasedOnItems(const ServerItem *item, size_t quantity);
  std::pair<const ServerItem *, size_t> strengthPair() const {
    return std::make_pair(_strength.item(), _strength.quantity());
  }

  void addYield(const ServerItem *item, double initMean, double initSD,
                size_t initMin, double gatherMean, double gatherSD);

  bool hasContainer() const { return _container != nullptr; }
  ContainerType &container() { return *_container; }
  const ContainerType &container() const { return *_container; }
  void addContainer(ContainerType *p) { _container = p; }

  bool hasDeconstruction() const { return _deconstruction != nullptr; }
  DeconstructionType &deconstruction() { return *_deconstruction; }
  const DeconstructionType &deconstruction() const { return *_deconstruction; }
  void addDeconstruction(DeconstructionType *p) { _deconstruction = p; }

  void action(Action *pAction) { _action = pAction; }
  void onDestroy(CallbackAction *pAction) { _onDestroy = pAction; }
  const Action &action() const { return *_action; }
  const CallbackAction &onDestroy() const { return *_onDestroy; }
  bool hasAction() const { return _action != nullptr; }
  bool hasOnDestroy() const { return _onDestroy != nullptr; }

  void initStrengthAndMaxHealth() const;

 private:
  void checkUniquenessInvariant() const;
};

#endif
