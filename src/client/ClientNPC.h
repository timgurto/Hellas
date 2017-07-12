#ifndef CLIENT_NPC_H
#define CLIENT_NPC_H

#include "ClientNPCType.h"
#include "ClientObject.h"

class ClientNPC : public ClientObject {

public:
    ClientNPC(size_t serial, const ClientNPCType *type = nullptr, const Point &loc = Point());
    ~ClientNPC(){}
    bool isFlat() const override;

    const ClientNPCType *npcType() const { return dynamic_cast<const ClientNPCType *>(type()); }

    char classTag() const override { return 'n'; }

    // From ClientCombatant:
    bool canBeAttackedByPlayer() const override;
    
    // From Sprite
    void onRightClick(Client &client) override;
    void draw(const Client &client) const;
    const Texture &cursor(const Client &client) const override;
};

#endif
