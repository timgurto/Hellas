#ifndef COMBATANT_PANEL_H
#define COMBATANT_PANEL_H

#include <string>

#include "Element.h"
#include "../ClientCombatant.h"

// A panel displaying a combatant's health bar, name, etc.
class CombatantPanel : public Element{
public:
    CombatantPanel(px_t x, px_t y,
                   const std::string &name, const health_t &health, const health_t &maxHealth);

    static const px_t
        WIDTH,
        HEIGHT,
        BAR_HEIGHT,
        GAP;
};

#endif
