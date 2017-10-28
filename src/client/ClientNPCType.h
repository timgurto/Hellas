#ifndef CLIENT_NPC_TYPE_H
#define CLIENT_NPC_TYPE_H

#include "ClientObject.h"

class SoundProfile;

class ClientNPCType : public ClientObjectType {

public:
    ClientNPCType(const std::string &id, Hitpoints maxHealth);
    virtual ~ClientNPCType() override{}


    virtual char classTag() const override { return 'n'; }
};

#endif
