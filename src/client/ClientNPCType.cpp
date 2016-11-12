#include "ClientNPC.h"
#include "ClientNPCType.h"
#include "Surface.h"

ClientNPCType::ClientNPCType(const std::string &id, health_t maxHealth):
ClientObjectType(id),
_maxHealth(maxHealth)
{
    containerSlots(ClientNPC::LOOT_CAPACITY);
}

void ClientNPCType::corpseImage(const std::string &filename){
    _corpseImage = Texture(filename, Color::MAGENTA);

    Surface corpseHighlightSurface(filename, Color::MAGENTA);
    if (!corpseHighlightSurface)
        return;

    // Recolor edges
    for (px_t x = 0; x != _corpseImage.width(); ++x)
        for (px_t y = 0; y != _corpseImage.height(); ++y){
            if (corpseHighlightSurface.getPixel(x, y) == Color::OUTLINE)
                corpseHighlightSurface.setPixel(x, y, Color::HIGHLIGHT_OUTLINE);
        }

    // Convert to Texture
    _corpseHighlightImage = Texture(corpseHighlightSurface);
}
