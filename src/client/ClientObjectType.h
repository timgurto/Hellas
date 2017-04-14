#ifndef CLIENT_OBJECT_TYPE_H
#define CLIENT_OBJECT_TYPE_H

#include <set>
#include <string>

#include "ClientItem.h"
#include "EntityType.h"
#include "Texture.h"
#include "../server/ItemSet.h"
#include "../Point.h"
#include "../util.h"

class SoundProfile;
class ParticleProfile;

// Describes a class of Entities, the "instances" of which share common properties
class ClientObjectType : public EntityType{
    struct ImageSet{
        Texture normal, highlight;
        ImageSet(){}
        ImageSet(const std::string &filename);
    };
    ImageSet _images; // baseline images, identical to Entity::_image and Entity::_highlightImage.

    std::string _id;
    std::string _name;
    bool _canGather; // Whether this represents objects that can be gathered
    std::string _gatherReq; // An item thus tagged is required to gather this object.
    std::string _constructionReq;
    bool _canDeconstruct; // Whether these objects can be deconstructed into items
    size_t _containerSlots;
    size_t _merchantSlots;
    Rect _collisionRect;
    const ParticleProfile *_gatherParticles;
    std::set<std::string> _tags;
    ItemSet _materials;
    mutable Texture *_materialsTooltip;
    ImageSet _constructionImage; // Shown when the object is under construction.
    const SoundProfile *_sounds;

    // To show transformations.  Which image is displayed depends on progress.
    std::vector<ImageSet> _transformImages;
    ms_t _transformTime; // The total length of the transformation.

public:
    ClientObjectType(const std::string &id);
    virtual ~ClientObjectType() override;

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
    size_t containerSlots() const { return _containerSlots; }
    void containerSlots(size_t n) { _containerSlots = n; }
    size_t merchantSlots() const { return _merchantSlots; }
    void merchantSlots(size_t n) { _merchantSlots = n; }
    const Rect &collisionRect() const { return _collisionRect; }
    void collisionRect(const Rect &r) { _collisionRect = r; }
    const ParticleProfile *gatherParticles() const { return _gatherParticles; }
    void gatherParticles(const ParticleProfile *profile) { _gatherParticles = profile; }
    void addMaterial(const ClientItem *item, size_t qty);
    const ItemSet &materials() const { return _materials; }
    bool hasTags() const { return !_tags.empty(); }
    const std::set<std::string> &tags() const { return _tags; }
    const Texture &materialsTooltip() const;
    bool transforms() const { return _transformTime > 0; }
    void transformTime(ms_t time) { _transformTime = time; }
    ms_t transformTime() const { return _transformTime; }
    void addTransformImage(const std::string &filename);
    const ImageSet &constructionImage() const { return _constructionImage; }
    void sounds(const std::string &id);
    const SoundProfile *sounds() const { return _sounds; }
    
    const ImageSet &getProgressImage(ms_t timeRemaining) const;

    virtual char classTag() const override { return 'o'; }
    virtual const Texture &image() const override { return _images.normal; }

    void addTag(const std::string &tagName){ _tags.insert(tagName); }

    struct ptrCompare{
        bool operator()(const ClientObjectType *lhs, const ClientObjectType *rhs){
            return lhs->_id < rhs->_id;
        }
    };
};

#endif
