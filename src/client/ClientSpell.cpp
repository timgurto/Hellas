#include <cassert>
#include <sstream>

#include "Client.h"
#include "ClientSpell.h"
#include "Tooltip.h"

ClientSpell::ClientSpell(const std::string &id) :
    _id(id),
    _castMessage(Client::compileMessage(CL_CAST, id)),
    _icon("Images/Spells/"s + id + ".png"s){
}

const Texture &ClientSpell::tooltip() const {
    if (_tooltip)
        return _tooltip;

    const auto &client = Client::instance();

    auto tb = Tooltip{};
    tb.setColor(Color::ITEM_NAME);
    tb.addLine(_name);

    tb.addGap();

    if (_school.isMagic()) {
        tb.setColor(_school.color());
        tb.addLine(_school);
    }

    tb.setColor(Color::ITEM_STATS);
    tb.addLine("Energy cost: "s + toString(_cost));
    
    if (!_isAoE)
        tb.addLine("Range: "s + toString(_range) + " podes"s);

    tb.setColor(Color::ITEM_INSTRUCTIONS);
    tb.addLine(createEffectDescription());

    _tooltip = tb.publish();
    return _tooltip;
}

std::string ClientSpell::createEffectDescription() const {
    std::ostringstream oss;

    auto targetString = _isAoE ?
        "all targets within "s + toString(_range) + " podes"s :
        "target"s;

    if (_effectName == "doDirectDamage")
        oss << "Deal " << _effectArgs.i1 << " damage to " << targetString << ".";

    else if (_effectName == "heal")
        oss << "Restore " << _effectArgs.i1 << " health to target.";

    else if (_effectName == "scaleThreat") {
        auto scalar = _effectArgs.d1;
        if (scalar < 1.0)
            oss << "Reduce your threat against target by " << toInt((1.0 - scalar) * 100.0) << "%.";
        else
            oss << "Increase your threat against target by " << toInt((scalar - 1.0) * 100.0) << "%.";
    }

    return oss.str();
}
