#include "Client.h"
#include "ClientCombatant.h"
#include "ClientCombatantType.h"
#include "Renderer.h"

extern Renderer renderer;

ClientCombatant::ClientCombatant(const ClientCombatantType *type):
_type(type),
_maxHealth(_type->maxHealth()),
_health(_maxHealth),
_maxEnergy(_type->maxEnergy()),
_energy(_maxEnergy)
{}

void ClientCombatant::drawHealthBarIfAppropriate(const Point &objectLocation, px_t objHeight) const{
    if (! shouldDrawHealthBar())
        return;

    static const px_t
        BAR_TOTAL_LENGTH = 10,
        BAR_HEIGHT = 2,
        BAR_GAP = 4; // Gap between the bar and the top of the sprite
    px_t
        barLength = toInt(1.0 * BAR_TOTAL_LENGTH * health() / maxHealth());
    const Point &offset = Client::_instance->offset();
    double
        x = objectLocation.x - toInt(BAR_TOTAL_LENGTH / 2) + offset.x,
        y = objectLocation.y - objHeight - BAR_GAP - BAR_HEIGHT + offset.y;

    renderer.setDrawColor(Color::HEALTH_BAR_OUTLINE);
    renderer.drawRect(Rect(x-1, y-1, BAR_TOTAL_LENGTH + 2, BAR_HEIGHT + 2));
    renderer.setDrawColor(healthBarColor());
    renderer.fillRect(Rect(x, y, barLength, BAR_HEIGHT));
    renderer.setDrawColor(Color::HEALTH_BAR_BACKGROUND);
    renderer.fillRect(Rect(x + barLength, y, BAR_TOTAL_LENGTH - barLength, BAR_HEIGHT));
}

bool ClientCombatant::shouldDrawHealthBar() const{
    if (! isAlive())
        return false;
    bool isDamaged = health() < maxHealth();
    if (isDamaged)
        return true;
    if (canBeAttackedByPlayer())
        return true;

    const Client &client = *Client::_instance;
    bool selected = client.targetAsCombatant() == this;
    bool mousedOver = client.currentMouseOverEntity() == entityPointer();
    if (selected || mousedOver)
        return true;

    return false;
}

void ClientCombatant::createDamageParticles() const{
    Client &client = *Client::_instance;
    client.addParticles(_type->damageParticles(), combatantLocation());
}
