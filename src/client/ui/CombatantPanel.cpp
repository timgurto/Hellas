#include "ColorBlock.h"
#include "CombatantPanel.h"
#include "LinkedLabel.h"
#include "ProgressBar.h"
#include "ShadowBox.h"

const px_t CombatantPanel::WIDTH = 60;
const px_t CombatantPanel::HEIGHT = 23;
const px_t CombatantPanel::BAR_HEIGHT = 7;
const px_t CombatantPanel::GAP = 2;

CombatantPanel::CombatantPanel(px_t x, px_t y, const std::string &name,
                               const health_t &health, const health_t &maxHealth):
Element(Rect(x, y, WIDTH, HEIGHT))
{
    addChild(new ColorBlock(Rect(0, 0, WIDTH, HEIGHT)));
    addChild(new ShadowBox(Rect(0, 0, WIDTH, HEIGHT)));
    addChild(new LinkedLabel<std::string>(
            Rect(GAP, 1, WIDTH - 2 * GAP, Element::TEXT_HEIGHT),
            name, "", "", Element::CENTER_JUSTIFIED));
    addChild(new ProgressBar<health_t>(
            Rect(2, HEIGHT - BAR_HEIGHT - GAP, WIDTH - 2 * GAP, BAR_HEIGHT),
            health, maxHealth, Color::COMBATANT_NEUTRAL));
}
