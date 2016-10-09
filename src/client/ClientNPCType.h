// (C) 2016 Tim Gurto

#ifndef CLIENT_NPC_TYPE_H
#define CLIENT_NPC_TYPE_H

#include "ClientObject.h"

class ClientNPCType : public ClientObjectType{
public:
    ClientNPCType(const std::string &id);

    virtual char classTag() const override { return 'n'; }
};

#endif
