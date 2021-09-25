#ifndef CLIENT_OBJECT_TYPE_H
#define CLIENT_OBJECT_TYPE_H

#include <set>
#include <string>

#include "../HasTags.h"
#include "../Point.h"
#include "../TerrainList.h"
#include "../server/ItemSet.h"
#include "../util.h"
#include "CQuest.h"
#include "ClientCombatantType.h"
#include "ClientItem.h"
#include "ClientObjectAction.h"
#include "HasSounds.h"
#include "SpriteType.h"
#include "Texture.h"
#include "drawPerItem.h"

class ConfirmationWindow;
class SoundProfile;
class ParticleProfile;

// Describes a class of Entities, the "instances" of which share common
// properties
class ClientObjectType : public SpriteType,
                         public ClientCombatantType,
                         public HasTags,
                         public HasSounds {
 public:
  using GatherChances = std::map<std::string, double>;

 private:
  std::string _id;
  std::string _name;
  std::string _imageFile;

  bool _canGather;  // Whether this represents objects that can be gathered
  std::string
      _gatherReq;  // An item thus tagged is required to gather this object.
  GatherChances _gatherChances;

  std::string _constructionReq;
  bool
      _canDeconstruct;  // Whether these objects can be deconstructed into items
  std::string _constructionText{};  // If present, replaces "under construction"
                                    // when items needed.
  size_t _containerSlots;
  size_t _merchantSlots;
  MapRect _collisionRect;
  bool _collides{false};
  const ParticleProfile *_gatherParticles;
  ItemSet _materials;
  mutable Optional<Tooltip> _constructionTooltip;
  ImageWithHighlight _constructionImage;  // Shown when under construction.
  bool _drawParticlesWhenUnderConstruction{false};
  Level _level{1};

  std::string _windowText;

 protected:
  ImageWithHighlight _corpseImage;

 private:
  bool _isPlayerUnique = false;

  std::string _allowedTerrain;

  RepairInfo _repairInfo;

  // To show transformations.  Which image is displayed depends on progress.
  std::vector<ImageWithHighlight> _transformImages;
  ms_t _transformTime;  // The total length of the transformation.

  ClientObjectAction *_action = nullptr;

  std::set<CQuest *> _startsQuests;
  std::set<CQuest *> _endsQuests;
  std::string _exclusiveToQuest{};

 public:
  ClientObjectType(const std::string &id);

  const std::string &id() const { return _id; }
  const std::string &name() const { return _name; }
  void name(const std::string &s) { _name = s; }
  const std::string &imageFile() const { return _imageFile; }
  void imageFile(const std::string &s) { _imageFile = s; }
  void setCorpseImage(const std::string &filename) {
    _corpseImage = {filename};
  }
  void setLevel(Level l) { _level = l; }
  Level level() const { return _level; }
  bool canGather(const CQuests &quests) const;
  void canGather(bool b) { _canGather = b; }
  const std::string &gatherReq() const { return _gatherReq; }
  void gatherReq(const std::string &req) { _gatherReq = req; }
  const std::string &constructionReq() const { return _constructionReq; }
  void constructionReq(const std::string &req) { _constructionReq = req; }
  bool canDeconstruct() const { return _canDeconstruct; }
  void canDeconstruct(bool b) { _canDeconstruct = b; }
  const std::string &constructionText() const { return _constructionText; }
  void constructionText(const std::string &text) { _constructionText = text; }
  size_t containerSlots() const { return _containerSlots; }
  void containerSlots(size_t n) { _containerSlots = n; }
  size_t merchantSlots() const { return _merchantSlots; }
  void merchantSlots(size_t n) { _merchantSlots = n; }
  const MapRect &collisionRect() const { return _collisionRect; }
  void collisionRect(const MapRect &r) {
    _collisionRect = r;
    _collides = true;
  }
  bool collides() const { return _collides; }
  void collides(bool b) { _collides = b; }
  const ParticleProfile *gatherParticles() const { return _gatherParticles; }
  void gatherParticles(const ParticleProfile *profile) {
    _gatherParticles = profile;
  }
  void chanceToGather(const std::string &itemID, double chance) {
    _gatherChances[itemID] = chance;
  }
  const GatherChances &gatherChances() const { return _gatherChances; }
  void addMaterial(const ClientItem *item, size_t qty);
  const ItemSet &materials() const { return _materials; }
  const Tooltip &constructionTooltip(const Client &client) const;
  void refreshConstructionTooltip() const {
    _constructionTooltip = Optional<Tooltip>{};
  }
  virtual void addClassSpecificStuffToConstructionTooltip(
      std::vector<std::string> &descriptionLines) const {}
  bool transforms() const { return _transformTime > 0; }
  void transformTime(ms_t time) { _transformTime = time; }
  ms_t transformTime() const { return _transformTime; }
  void addTransformImage(const std::string &filename);
  const ImageWithHighlight &constructionImage() const {
    return _constructionImage;
  }
  const RepairInfo &repairInfo() const { return _repairInfo; }
  void makeRepairable() { _repairInfo.canBeRepaired = true; }
  void repairingCosts(const std::string &id) { _repairInfo.cost = id; }
  void repairingRequiresTool(const std::string &tag) { _repairInfo.tool = tag; }
  bool isPlayerUnique() const { return _isPlayerUnique; }
  void makePlayerUnique() { _isPlayerUnique = true; }
  void action(ClientObjectAction *pAction) { _action = pAction; }
  const ClientObjectAction &action() const { return *_action; }
  bool hasAction() const { return _action != nullptr; }
  void startsQuest(CQuest &quest) { _startsQuests.insert(&quest); }
  std::set<CQuest *> &startsQuests() { return _startsQuests; }
  const std::set<CQuest *> &startsQuests() const { return _startsQuests; }
  void endsQuest(CQuest &quest) { _endsQuests.insert(&quest); }
  std::set<CQuest *> &endsQuests() { return _endsQuests; }
  const std::set<CQuest *> &endsQuests() const { return _endsQuests; }
  void exclusiveToQuest(const std::string &questID) {
    _exclusiveToQuest = questID;
  }
  const std::string &exclusiveToQuest() const { return _exclusiveToQuest; }
  void allowedTerrain(const std::string &terrainList) {
    _allowedTerrain = terrainList;
  }
  const std::string &allowedTerrain() const {
    if (_allowedTerrain.empty()) return TerrainList::defaultList().id();
    return _allowedTerrain;
  };
  void setWindowText(const std::string &text) { _windowText = text; }
  const std::string &windowText() const { return _windowText; }
  void drawParticlesWhenUnderConstruction() {
    _drawParticlesWhenUnderConstruction = true;
  }
  bool shouldDrawParticlesWhenUnderConstruction() const {
    return _drawParticlesWhenUnderConstruction;
  }
  std::string onlyAllowedItemInContainer;
  DrawPerItemTypeInfo drawPerItemInfo;

  const ImageWithHighlight &getProgressImage(ms_t timeRemaining) const;
  const Texture &corpseImage() const { return _corpseImage.getNormalImage(); }
  const Texture &corpseHighlightImage() const {
    return _corpseImage.getHighlightImage();
  }
  void initialiseHumanoidCorpse();

  virtual char classTag() const override { return 'o'; }

  struct ptrCompare {
    bool operator()(const ClientObjectType *lhs,
                    const ClientObjectType *rhs) const {
      return lhs->_id < rhs->_id;
    }
  };
};

#endif
