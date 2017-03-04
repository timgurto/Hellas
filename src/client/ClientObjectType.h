#ifndef CLIENT_OBJECT_TYPE_H
#define CLIENT_OBJECT_TYPE_H

#include <set>
#include <string>

#include "EntityType.h"
#include "Texture.h"
#include "../Point.h"
#include "../util.h"

struct Mix_Chunk;
class ParticleProfile;

// Describes a class of Entities, the "instances" of which share common properties
class ClientObjectType : public EntityType{
    std::string _id;
    std::string _name;
    bool _canGather; // Whether this represents objects that can be gathered
    std::string _gatherReq; // An item thus tagged is required to gather this object.
    std::string _constructionReq;
    bool _canDeconstruct; // Whether these objects can be deconstructed into items
    size_t _containerSlots;
    size_t _merchantSlots;
    Mix_Chunk *_gatherSound;
    Rect _collisionRect;
    const ParticleProfile *_gatherParticles;
    std::set<std::string> _tags;

public:
    ClientObjectType(const std::string &id);
    virtual ~ClientObjectType();

    const std::string &id() const { return _id; }
    const std::string &name() const { return _name; }
    void name(const std::string &s) { _name = s; }
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
    void gatherSound(const std::string &filename);
    Mix_Chunk *gatherSound() const { return _gatherSound; }
    const Rect &collisionRect() const { return _collisionRect; }
    void collisionRect(const Rect &r) { _collisionRect = r; }
    const ParticleProfile *gatherParticles() const { return _gatherParticles; }
    void gatherParticles(const ParticleProfile *profile) { _gatherParticles = profile; }
    bool hasTags() const { return !_tags.empty(); }
    const std::set<std::string> &tags() const { return _tags; }

    virtual char classTag() const override { return 'o'; }

    void addTag(const std::string &tagName){ _tags.insert(tagName); }

    struct ptrCompare{
        bool operator()(const ClientObjectType *lhs, const ClientObjectType *rhs){
            return lhs->_id < rhs->_id;
        }
    };
};

#endif
