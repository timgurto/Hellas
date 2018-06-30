#ifndef CLIENT_OBJECT_TYPE_H
#define CLIENT_OBJECT_TYPE_H

#include <set>
#include <string>

#include "../Point.h"
#include "../server/ItemSet.h"
#include "../util.h"
#include "ClientCombatantType.h"
#include "ClientItem.h"
#include "ClientObjectAction.h"
#include "SpriteType.h"
#include "Texture.h"

class ConfirmationWindow;
class SoundProfile;
class ParticleProfile;

// Describes a class of Entities, the "instances" of which share common
// properties
class ClientObjectType : public SpriteType, public ClientCombatantType {
  struct ImageSet {
    Texture normal, highlight;
    ImageSet() {}
    ImageSet(const std::string &filename);
  };
  ImageSet _images;  // baseline images, identical to Sprite::_image and
                     // Sprite::_highlightImage.

  std::string _id;
  std::string _name;
  bool _canGather;  // Whether this represents objects that can be gathered
  std::string
      _gatherReq;  // An item thus tagged is required to gather this object.
  std::string _constructionReq;
  bool
      _canDeconstruct;  // Whether these objects can be deconstructed into items
  std::string _constructionText{};  // If present, replaces "under construction"
                                    // when items needed.
  size_t _containerSlots;
  size_t _merchantSlots;
  MapRect _collisionRect;
  const ParticleProfile *_gatherParticles;
  std::set<std::string> _tags;
  ItemSet _materials;
  mutable Optional<Tooltip> _constructionTooltip;
  ImageSet _constructionImage;  // Shown when the object is under construction.
  Texture _corpseImage, _corpseHighlightImage;
  const SoundProfile *_sounds;
  bool _isPlayerUnique = false;

  struct Strength {
    Strength() : item(nullptr), quantity(0) {}
    const ClientItem *item;
    size_t quantity;
  };
  Strength _strength;

  // To show transformations.  Which image is displayed depends on progress.
  std::vector<ImageSet> _transformImages;
  ms_t _transformTime;  // The total length of the transformation.

  ClientObjectAction *_action = nullptr;

 public:
  ClientObjectType(const std::string &id);

  const std::string &id() const { return _id; }
  const std::string &name() const { return _name; }
  void name(const std::string &s) { _name = s; }
  void imageSet(const std::string &fileName) { _images = ImageSet(fileName); }
  bool canGather() const { return _canGather; }
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
  void collisionRect(const MapRect &r) { _collisionRect = r; }
  const ParticleProfile *gatherParticles() const { return _gatherParticles; }
  void gatherParticles(const ParticleProfile *profile) {
    _gatherParticles = profile;
  }
  void addMaterial(const ClientItem *item, size_t qty);
  const ItemSet &materials() const { return _materials; }
  bool hasTags() const { return !_tags.empty(); }
  const std::set<std::string> &tags() const { return _tags; }
  const Tooltip &constructionTooltip() const;
  bool transforms() const { return _transformTime > 0; }
  void transformTime(ms_t time) { _transformTime = time; }
  ms_t transformTime() const { return _transformTime; }
  void addTransformImage(const std::string &filename);
  const ImageSet &constructionImage() const { return _constructionImage; }
  void sounds(const std::string &id);
  const SoundProfile *sounds() const { return _sounds; }
  void strength(const ClientItem *item, size_t quantity) {
    _strength.item = item;
    _strength.quantity = quantity;
  }
  bool isPlayerUnique() const { return _isPlayerUnique; }
  void makePlayerUnique() { _isPlayerUnique = true; }
  void action(ClientObjectAction *pAction) { _action = pAction; }
  const ClientObjectAction &action() const { return *_action; }
  bool hasAction() const { return _action != nullptr; }

  const ImageSet &getProgressImage(ms_t timeRemaining) const;
  void corpseImage(const std::string &filename);
  const Texture &corpseImage() const { return _corpseImage; }
  const Texture &corpseHighlightImage() const { return _corpseHighlightImage; }

  virtual char classTag() const override { return 'o'; }
  virtual const Texture &image() const override { return _images.normal; }

  void addTag(const std::string &tagName) { _tags.insert(tagName); }

  void calculateAndInitStrength();

  struct ptrCompare {
    bool operator()(const ClientObjectType *lhs,
                    const ClientObjectType *rhs) const {
      return lhs->_id < rhs->_id;
    }
  };
};

#endif
