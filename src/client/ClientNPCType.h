#ifndef CLIENT_NPC_TYPE_H
#define CLIENT_NPC_TYPE_H

#include "ClientObject.h"
#include "Projectile.h"

class SoundProfile;

class ClientNPCType : public ClientObjectType {

public:
    ClientNPCType(const std::string &id, const std::string &imagePath, Hitpoints maxHealth);
    virtual ~ClientNPCType() override{}

    void projectile(const Projectile::Type &type) { _projectile = &type; }
    const Projectile::Type *projectile() const { return _projectile; }

    void addGear(const ClientItem &item);
    const ClientItem *gear(size_t slot) const;
    bool hasGear() const { return !_gear.empty(); }

    virtual char classTag() const override { return 'n';} 

private:
    const Projectile::Type *_projectile = nullptr;
    using Gear = std::vector<const ClientItem *>;
    Gear _gear; // For humanoid NPCs
};

#endif
