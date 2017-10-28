#include "ColorBlock.h"
#include "CombatantPanel.h"
#include "LinkedLabel.h"
#include "ShadowBox.h"

const px_t CombatantPanel::WIDTH = 60;
const px_t CombatantPanel::HEIGHT = 40;
const px_t CombatantPanel::BAR_HEIGHT = 7;
const px_t CombatantPanel::GAP = 2;

CombatantPanel::CombatantPanel(px_t panelX, px_t panelY, const std::string &name,
    const Hitpoints &health, const Hitpoints &maxHealth,
    const Energy &energy, const Energy &maxEnergy):
    Element({ panelX, panelY, WIDTH, HEIGHT })
{
    addChild(new ColorBlock({ 0, 0, WIDTH, HEIGHT }));
    addChild(new ShadowBox({ 0, 0, WIDTH, HEIGHT }));

    const auto
        ELEMENT_WIDTH = WIDTH - GAP * 2;
    auto y = GAP;
    addChild(new LinkedLabel<std::string>({ GAP, y, ELEMENT_WIDTH, Element::TEXT_HEIGHT },
        name, {}, {}, Element::CENTER_JUSTIFIED));
    y += Element::TEXT_HEIGHT + GAP;

    _healthBar = new ProgressBar<Hitpoints>({ GAP, y, ELEMENT_WIDTH, BAR_HEIGHT },
            health, maxHealth, Color::COMBATANT_NEUTRAL);
    addChild(_healthBar);
    y += BAR_HEIGHT + GAP;

    _energyBar = new ProgressBar<Hitpoints>({ GAP, y, ELEMENT_WIDTH, BAR_HEIGHT },
        energy, maxEnergy, Color::COMBATANT_NEUTRAL);
    addChild(_energyBar);
    y += BAR_HEIGHT + GAP;

    this->height(y);
}
