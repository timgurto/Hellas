// (C) 2016 Tim Gurto

#include "Client.h"
#include "ClientNPC.h"

extern Renderer renderer;

ClientNPC::ClientNPC(size_t serial, const ClientNPCType *type, const Point &loc):
ClientObject(serial, type, loc),
_health(type->maxHealth())
{}

void ClientNPC::draw(const Client &client) const{
    ClientObject::draw(client);

    // Draw health bar if damaged or targeted
    if (client.targetNPC() == this || _health < npcType()->maxHealth()){
        static const px_t
            BAR_TOTAL_LENGTH = 10,
            BAR_HEIGHT = 2,
            BAR_GAP = 0; // Gap between the bar and the top of the sprite
        px_t
            barLength = toInt(1.0 * BAR_TOTAL_LENGTH * _health / npcType()->maxHealth());
        double
            x = location().x - toInt(BAR_TOTAL_LENGTH / 2),
            y = drawRect().y - BAR_GAP - BAR_HEIGHT;
        static const Color
            OUTLINE = Color::MMO_OUTLINE,
            COLOR = Color::MMO_L_GREEN,
            BACKGROUND_COLOR = Color::MMO_RED;
        const Point &offset = client.offset();
        renderer.setDrawColor(Color::MMO_OUTLINE);
        renderer.drawRect(Rect(x-1 + offset.x, y-1 + offset.y, BAR_TOTAL_LENGTH + 2, BAR_HEIGHT + 2));
        renderer.setDrawColor(COLOR);
        renderer.fillRect(Rect(x, y, barLength, BAR_HEIGHT) + offset);
        renderer.setDrawColor(BACKGROUND_COLOR);
        renderer.fillRect(Rect(x + barLength, y, BAR_TOTAL_LENGTH - barLength, BAR_HEIGHT) + offset);
    }
}
