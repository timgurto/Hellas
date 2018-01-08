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

    auto effectName = _effectName;
    auto effectArgs = _effectArgs;

    auto buffDuration = 0;
    auto statsBuff = false;
    const ClientBuffType *buff = nullptr;

    auto isBuff = _effectName == "buff" || _effectName == "debuff";
    if (isBuff) {
        auto buffName = effectArgs.s1;
        auto it = Client::instance().buffTypes().find(buffName);
        assert(it != Client::instance().buffTypes().end());
        buff = &it->second;

        buffDuration = buff->duration();

        if (!buff->effectName().empty()) {
            effectName = buff->effectName();
            effectArgs = buff->effectArgs();

            if (buff->tickTime() > 0) {
                auto numTicks = buff->duration() * 1000 / buff->tickTime();
                effectArgs.i1 *= numTicks; // "format: n over m seconds"
            }
        } else { // Assuming stats.  This will need to be changed when, for example, stun debuffs are added
            statsBuff = true; // In lieu of an effect name
        }
    }

    auto targetString = _isAoE ?
        "all targets within "s + toString(_range) + " podes"s :
        "target"s;

    if (statsBuff) {
        oss << "Grant ";
        auto statStrings = buff->stats().toStrings();
        auto firstStringHasBeenPrinted = false;
        for (auto &statString : statStrings) {
            if (firstStringHasBeenPrinted)
                oss << ", ";
            oss << statString;
            firstStringHasBeenPrinted = true;
        }
        oss << " to " << targetString;
    }

    else if (effectName == "doDirectDamage")
        oss << "Deal " << effectArgs.i1 << " damage to " << targetString;

    else if (effectName == "heal")
        oss << "Restore " << effectArgs.i1 << " health to target";

    else if (effectName == "scaleThreat") {
        auto scalar = effectArgs.d1;
        if (scalar < 1.0)
            oss << "Reduce your threat against target by " << toInt((1.0 - scalar) * 100.0) << "%";
        else
            oss << "Increase your threat against target by " << toInt((scalar - 1.0) * 100.0) << "%";
    }

    if (isBuff) {
        auto conjunction = statsBuff ? "for"s : "over"s;
        
        oss << " " << conjunction << " " << sAsTimeDisplay(buffDuration);
    }

    oss << ".";

    return oss.str();
}
