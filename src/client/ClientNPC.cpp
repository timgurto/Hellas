#include <cassert>

#include "Client.h"
#include "ClientNPC.h"
#include "ui/TakeContainer.h"

extern Renderer renderer;

const size_t ClientNPC::LOOT_CAPACITY = 8;

ClientNPC::ClientNPC(size_t serial, const ClientNPCType *type, const Point &loc):
ClientObject(serial, type, loc),
ClientCombatant(type),
_lootable(false),
_lootContainer(nullptr)
{}

void ClientNPC::draw(const Client &client) const{
    ClientObject::draw(client);

    drawHealthBarIfAppropriate(location(), height());

    if (isDebug()) {
        renderer.setDrawColor(Color::WHITE);
        renderer.drawRect(collisionRect() + client.offset());
        renderer.setDrawColor(Color::YELLOW);
        renderer.fillRect(Rect(location().x + client.offset().x,
                                location().y + client.offset().y - 1,
                                1, 3));
        renderer.fillRect(Rect(location().x + client.offset().x - 1,
                               location().y + client.offset().y,
                               3, 1));
    }
}

void ClientNPC::update(double delta){
    ClientObject::update(delta);

    // Loot sparkles
    if (lootable())
        Client::_instance->addParticles("lootSparkles", location(), delta);
}

void ClientNPC::onLeftClick(Client &client){
    client.setTarget(*this);
    
    // Note: parent class's onLeftClick() not called.
}

void ClientNPC::onRightClick(Client &client){
    client.setTarget(*this, true);
    
    // Loot window
    if (lootable())
        ClientObject::onRightClick(client);
}

void ClientNPC::assembleWindow(Client &client){
    static const px_t
        WIDTH = 100,
        HEIGHT = 100;
    _lootContainer = new TakeContainer(container(), serial(), Rect(0, 0, WIDTH, HEIGHT));
    Rect winRect = location();
    winRect.w = WIDTH;
    winRect.h = HEIGHT;
    _window = new Window(winRect, objectType()->name());
    _window->addChild(_lootContainer);
}

const Texture &ClientNPC::cursor(const Client &client) const{
    if (isAlive())
        return client.cursorAttack();
    if (lootable())
        return  client.cursorContainer();
    return client.cursorNormal();
}

void ClientNPC::onInventoryUpdate(){
    if (_lootContainer != nullptr)
        _lootContainer->repopulate();
}

bool ClientNPC::isFlat() const{
    return Sprite::isFlat() || health() == 0;
}

const Texture &ClientNPC::image() const{
    return (health() > 0) ? ClientObject::image() : npcType()->corpseImage();
}

const Texture &ClientNPC::highlightImage() const{
    return (health() > 0) ? ClientObject::highlightImage() : npcType()->corpseHighlightImage();
}

void ClientNPC::sendTargetMessage() const{
    const Client &client = *Client::_instance;
    client.sendMessage(CL_TARGET_ENTITY, makeArgs(serial()));
}
