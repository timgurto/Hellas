#include <cassert>
#include <sstream>

#include "Client.h"
#include "ClientSpell.h"
#include "TooltipBuilder.h"

ClientSpell::ClientSpell(const std::string &id) :
    _castMessage(Client::compileMessage(CL_CAST, id)),
    _icon("Images/Spells/"s + id + ".png"s){
}

const Texture &ClientSpell::tooltip() const {
    if (_tooltip)
        return _tooltip;

    const auto &client = Client::instance();

    auto tb = TooltipBuilder{};
    tb.setColor(Color::ITEM_NAME);
    tb.addLine(_name);

    tb.addGap();

    tb.setColor(Color::ITEM_STATS);
    tb.addLine("Energy cost: "s + toString(_cost));
    tb.addLine("Range: "s + toString(_range) + " podes"s);

    tb.setColor(Color::ITEM_INSTRUCTIONS);
    tb.addLine(createEffectDescription());

    _tooltip = tb.publish();
    return _tooltip;
}

std::string ClientSpell::createEffectDescription() const {
    std::ostringstream oss;

    if (_effectName == "doDirectDamage")
        oss << "Deals " << _effectArgs[0] << " damage to target.";

    else if (_effectName == "heal")
        oss << "Restores " << _effectArgs[0] << " health to target.";

    return oss.str();
}
