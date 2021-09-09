#ifndef OBJECT_TYPE_H
#define OBJECT_TYPE_H

#include "../../types.h"
#include "../EntityType.h"
#include "../QuestNode.h"
#include "../Transformation.h"
#include "Action.h"
#include "Container.h"
#include "Deconstruction.h"

class ServerItem;
class BuffType;

// Describes a class of Objects, the "instances" of which share common
// properties
class ObjectType : public EntityType, public QuestNodeType {
  RepairInfo _repairInfo;

  mutable size_t _numInWorld;

  std::string _constructionReq;
  ms_t _constructionTime;
  bool _isUnique;       // Can only exist once at a time in the world.
  bool _isUnbuildable;  // Data suggests it can be built, but direct
                        // construction should be blocked.

  size_t _merchantSlots;
  bool _bottomlessMerchant;  // Bottomless: never runs out, uses no inventory
                             // space.

  ms_t _disappearsAfter{
      0};  // After an object has existed for this long, it disappears.

  bool _destroyIfUsedAsTool{false};

  ItemSet _materials;  // The necessary materials, if this needs to be
                       // constructed in-place.
  bool _knownByDefault;

  std::string
      _playerUniqueCategory;  // Assumption: up to one category per object type.

  std::string _exclusiveToQuest{};

  const BuffType *_buffGranted{
      nullptr};  // A buff granted to nearby, permitted users.
  double _buffRadius{0};

  bool _isGate{false};

 protected:
  ContainerType *_container;
  DeconstructionType *_deconstruction;

  Action *_action = nullptr;
  CallbackAction *_onDestroy = nullptr;

 public:
  ObjectType(const std::string &id);

  virtual ~ObjectType() {}

  virtual void initialise() const;

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
  void exclusiveToQuest(const std::string &questID) {
    _exclusiveToQuest = questID;
  }
  const std::string &exclusiveToQuest() const { return _exclusiveToQuest; }
  void markAsGate() { _isGate = true; }
  bool isGate() const { return _isGate; }

  virtual char classTag() const override { return 'o'; }

  ms_t constructionTime() const { return _constructionTime; }
  void constructionTime(ms_t t) { _constructionTime = t; }
  void addMaterial(const Item *material, size_t quantity) {
    _materials.add(material, quantity);
  }
  const ItemSet &materials() const { return _materials; }
  void disappearsAfter(ms_t time) { _disappearsAfter = time; }
  ms_t disappearsAfter() const { return _disappearsAfter; }

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

  void grantsBuff(const BuffType *buff, double radius) {
    _buffGranted = buff;
    _buffRadius = radius;
  }
  bool grantsBuff() const { return _buffGranted != nullptr; }
  const BuffType *buffGranted() const { return _buffGranted; }
  double buffRadius() const { return _buffRadius; }

  const RepairInfo &repairInfo() const { return _repairInfo; }
  void makeRepairable() { _repairInfo.canBeRepaired = true; }
  void repairingCosts(const std::string &id) { _repairInfo.cost = id; }
  void repairingRequiresTool(const std::string &tag) { _repairInfo.tool = tag; }
  void destroyIfUsedAsTool(bool b) { _destroyIfUsedAsTool = b; }
  bool destroyIfUsedAsTool() const { return _destroyIfUsedAsTool; }

 private:
  void checkUniquenessInvariant() const;

 public:
};

#endif
