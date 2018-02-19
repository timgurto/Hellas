#include "ClientConfig.h"
#include "../XmlReader.h"

void ClientConfig::loadFromFile(const std::string &filename) {
    XmlReader xr("client-config.xml");

    auto elem = xr.findChild("chatLog");
    xr.findAttr(elem, "width", chatW);
    xr.findAttr(elem, "height", chatH);

    elem = xr.findChild("colors");
    xr.findAttr(elem, "warning", Color::WARNING);
    xr.findAttr(elem, "failure", Color::FAILURE);
    xr.findAttr(elem, "success", Color::SUCCESS);
    xr.findAttr(elem, "chatLogBackground", Color::CHAT_LOG_BACKGROUND);
    xr.findAttr(elem, "say", Color::SAY);
    xr.findAttr(elem, "whisper", Color::WHISPER);
    xr.findAttr(elem, "defaultDraw", Color::DEFAULT_DRAW);
    xr.findAttr(elem, "font", Color::FONT);
    xr.findAttr(elem, "fontOutline", Color::FONT_OUTLINE);
    xr.findAttr(elem, "disabledText", Color::DISABLED_TEXT);
    xr.findAttr(elem, "tooltipFont", Color::TOOLTIP_FONT);
    xr.findAttr(elem, "tooltipBackground", Color::TOOLTIP_BACKGROUND);
    xr.findAttr(elem, "tooltipBorder", Color::TOOLTIP_BORDER);
    xr.findAttr(elem, "flavourText", Color::FLAVOUR_TEXT);
    xr.findAttr(elem, "elementBackground", Color::ELEMENT_BACKGROUND);
    xr.findAttr(elem, "elementShadowDark", Color::ELEMENT_SHADOW_DARK);
    xr.findAttr(elem, "elementShadowLight", Color::ELEMENT_SHADOW_LIGHT);
    xr.findAttr(elem, "elementFont", Color::ELEMENT_FONT);
    xr.findAttr(elem, "containerSlotBackground", Color::CONTAINER_SLOT_BACKGROUND);
    xr.findAttr(elem, "itemName", Color::ITEM_NAME);
    xr.findAttr(elem, "itemStats", Color::ITEM_STATS);
    xr.findAttr(elem, "itemInstructions", Color::ITEM_INSTRUCTIONS);
    xr.findAttr(elem, "itemTags", Color::ITEM_TAGS);
    xr.findAttr(elem, "helpTextHeading", Color::HELP_TEXT_HEADING);
    xr.findAttr(elem, "footprintGood", Color::FOOTPRINT_GOOD);
    xr.findAttr(elem, "footprintBad", Color::FOOTPRINT_BAD);
    xr.findAttr(elem, "footprint", Color::FOOTPRINT);
    xr.findAttr(elem, "inRange", Color::IN_RANGE);
    xr.findAttr(elem, "outOfRange", Color::OUT_OF_RANGE);
    xr.findAttr(elem, "healthBarBackground", Color::HEALTH_BAR_BACKGROUND);
    xr.findAttr(elem, "healthBarOutline", Color::HEALTH_BAR_OUTLINE);
    xr.findAttr(elem, "performanceFont", Color::PERFORMANCE_FONT);
    xr.findAttr(elem, "castBarFont", Color::CAST_BAR_FONT);
    xr.findAttr(elem, "progressBar", Color::PROGRESS_BAR);
    xr.findAttr(elem, "progressBarBackground", Color::PROGRESS_BAR_BACKGROUND);
    xr.findAttr(elem, "combatantSelf", Color::COMBATANT_SELF);
    xr.findAttr(elem, "combatantAlly", Color::COMBATANT_ALLY);
    xr.findAttr(elem, "combatantNeutral", Color::COMBATANT_NEUTRAL);
    xr.findAttr(elem, "combatantEnemy", Color::COMBATANT_ENEMY);
    xr.findAttr(elem, "energy", Color::ENERGY);
    xr.findAttr(elem, "playerNameOutline", Color::PLAYER_NAME_OUTLINE);
    xr.findAttr(elem, "outline", Color::OUTLINE);
    xr.findAttr(elem, "highlightOutline", Color::HIGHLIGHT_OUTLINE);
    xr.findAttr(elem, "floatingDamage", Color::FLOATING_DAMAGE);
    xr.findAttr(elem, "floatingMiss", Color::FLOATING_MISS);
    xr.findAttr(elem, "air", Color::AIR);
    xr.findAttr(elem, "earth", Color::EARTH);
    xr.findAttr(elem, "fire", Color::FIRE);
    xr.findAttr(elem, "water", Color::WATER);

    elem = xr.findChild("gameFont");
    xr.findAttr(elem, "filename", fontFile);
    xr.findAttr(elem, "size", fontSize);
    xr.findAttr(elem, "offset", fontOffset);
    xr.findAttr(elem, "height", textHeight);

    elem = xr.findChild("castBar");
    xr.findAttr(elem, "y", castBarY);
    xr.findAttr(elem, "w", castBarW);
    xr.findAttr(elem, "h", castBarH);

    elem = xr.findChild("loginScreen");
    xr.findAttr(elem, "frontX", loginFrontOffset.x);
    xr.findAttr(elem, "frontY", loginFrontOffset.y);

    elem = xr.findChild("server");
    xr.findAttr(elem, "hostDirectory", serverHostDirectory);
}
