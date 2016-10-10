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
        static const double HP_PER_PIXELS = 1;
        static const px_t
            BAR_HEIGHT = 2,
            BAR_GAP = 0;
        px_t
            barTotalWidth = max<px_t>(1, toInt(npcType()->maxHealth() / HP_PER_PIXELS)),
            barWidth = toInt(_health / HP_PER_PIXELS),
            x = location().x - toInt(barWidth / 2),
            y = drawRect().y - BAR_GAP - BAR_HEIGHT;
        static const Color
            OUTLINE = Color::MMO_OUTLINE,
            COLOR = Color::MMO_L_GREEN,
            BACKGROUND_COLOR = Color::MMO_RED;
        const Point &offset = client.offset();
        renderer.setDrawColor(Color::MMO_OUTLINE);
        renderer.fillRect(Rect(x-1 + offset.x, y-1 + offset.y, barTotalWidth + 2, BAR_HEIGHT + 2));
        renderer.setDrawColor(COLOR);
        renderer.fillRect(Rect(x, y, barWidth, BAR_HEIGHT) + offset);
        renderer.setDrawColor(BACKGROUND_COLOR);
        renderer.fillRect(Rect(x + barWidth, y, barTotalWidth - barWidth, BAR_HEIGHT) + offset);
    }
}
