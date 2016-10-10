// (C) 2016 Tim Gurto

#ifndef CLIENT_NPC_H
#define CLIENT_NPC_H

#include "ClientNPCType.h"
#include "ClientObject.h"

class ClientNPC : public ClientObject{
    health_t _health;

public:
    ClientNPC(size_t serial, const ClientNPCType *type = nullptr, const Point &loc = Point());

    health_t health() const { return _health; }
    void health(health_t n) { _health = n; }

    const ClientNPCType *npcType() const { return dynamic_cast<const ClientNPCType *>(type()); }

    virtual char classTag() const override { return 'n'; }
    
    virtual void onLeftClick(Client &client) override;
    virtual void onRightClick(Client &client) override;
    virtual void draw(const Client &client) const;
};

#endif
