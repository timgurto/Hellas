#include "Client.h"
#include "ClientNPC.h"

extern Renderer renderer;

const size_t ClientNPC::LOOT_CAPACITY = 8;

ClientNPC::ClientNPC(size_t serial, const ClientNPCType *type, const Point &loc):
ClientObject(serial, type, loc),
_health(type->maxHealth()),
_lootable(false)
{}

void ClientNPC::draw(const Client &client) const{
    const Point &offset = client.offset();

    // Draw the sprite here, rather than calling Entity::draw()
    const Texture image = _health > 0 ? type()->image() : npcType()->corpseImage();
    image.draw(location() + offset + type()->drawRect());

    // Draw health bar if damaged or targeted
    if (health() == 0)
        return;
    if (client.targetNPC() == this || client.currentMouseOverEntity() == this ||
        _health < npcType()->maxHealth()){
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
        renderer.setDrawColor(Color::MMO_OUTLINE);
        renderer.drawRect(Rect(x-1 + offset.x, y-1 + offset.y, BAR_TOTAL_LENGTH + 2, BAR_HEIGHT + 2));
        renderer.setDrawColor(COLOR);
        renderer.fillRect(Rect(x, y, barLength, BAR_HEIGHT) + offset);
        renderer.setDrawColor(BACKGROUND_COLOR);
        renderer.fillRect(Rect(x + barLength, y, BAR_TOTAL_LENGTH - barLength, BAR_HEIGHT) + offset);
    }
}

void ClientNPC::update(double delta){
    ClientObject::update(delta);

    // Loot sparkles
    if (lootable())
        Client::_instance->addParticles("lootSparkles", location(), delta);
}

void ClientNPC::onLeftClick(Client &client){
    client.targetNPC(this);
    
    // Note: parent class's onLeftClick() not called.
}

void ClientNPC::onRightClick(Client &client){
    client.targetNPC(this, true);
    
    // Loot window
    if (lootable())
        ClientObject::onRightClick(client);
}

const Texture &ClientNPC::cursor(const Client &client) const{
    if (_health > 0)
        return client.cursorAttack();
    if (lootable())
        return  client.cursorContainer();
    return client.cursorNormal();
}
