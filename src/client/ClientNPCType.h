#ifndef CLIENT_NPC_TYPE_H
#define CLIENT_NPC_TYPE_H

#include "ClientObject.h"

class ClientNPCType : public ClientObjectType{
    health_t _maxHealth;
    Texture _corpseImage, _corpseHighlightImage;

public:
    ClientNPCType(const std::string &id, health_t maxHealth);
    virtual ~ClientNPCType() override{}

    health_t maxHealth() const { return _maxHealth; }
    void maxHealth(health_t n) { _maxHealth = n; }
    void corpseImage(const std::string &filename);
    const Texture &corpseImage() const { return _corpseImage; }
    const Texture &corpseHighlightImage() const { return _corpseHighlightImage; }

    virtual char classTag() const override { return 'n'; }
};

#endif
