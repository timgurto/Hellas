// (C) 2016 Tim Gurto

#ifndef CLIENT_NPC_H
#define CLIENT_NPC_H

#include "ClientNPCType.h"
#include "ClientObject.h"

class ClientNPC : public ClientObject{
public:
    ClientNPC(size_t serial, const ClientNPCType *type = nullptr, const Point &loc = Point());

    virtual char classTag() const override { return 'n'; }
};

#endif
