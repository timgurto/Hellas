#include "Client.h"
#include "ClientNPC.h"
#include "ClientNPCType.h"
#include "Surface.h"

ClientNPCType::ClientNPCType(const std::string &id, const std::string &imagePath,
        Hitpoints maxHealthArg):
ClientObjectType(id)
{
    maxHealth(maxHealthArg);
    Client &client = *Client::_instance;
    damageParticles(client.findParticleProfile("blood"));

    setImage(imagePath + ".png");
    imageSet(imagePath + ".png");
    corpseImage(imagePath + "-corpse.png");
}
