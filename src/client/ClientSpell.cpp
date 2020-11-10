#include "ClientSpell.h"

#include <cassert>
#include <sstream>

#include "Client.h"
#include "Tooltip.h"

ClientSpell::ClientSpell(const std::string &id, Client &client)
    : _id(id), _client(client), _castMessage(CL_CAST_SPELL, id) {}

void ClientSpell::icon(const Texture &iconTexture) { _icon = iconTexture; }

const Tooltip &ClientSpell::tooltip() const {
  if (_tooltip.hasValue()) return _tooltip.value();

  _tooltip = Tooltip{};
  auto &tooltip = _tooltip.value();
  tooltip.setColor(Color::TOOLTIP_NAME);
  tooltip.addLine(_name);

  tooltip.addGap();

  if (_school.isMagic()) {
    tooltip.setColor(_school.color());
    tooltip.addLine(_school);
  }

  tooltip.setColor(Color::TOOLTIP_BODY);
  if (_cost > 0) tooltip.addLine("Energy cost: "s + toString(_cost));

  auto shouldShowRange = !_isAoE && _range > 0;
  if (shouldShowRange)
    tooltip.addLine("Range: "s + toString(_range) + " podes"s);

  if (_cooldown > 0) tooltip.addLine("Cooldown: "s + sAsTimeDisplay(_cooldown));

  tooltip.setColor(Color::TOOLTIP_BODY);
  tooltip.addLine(createEffectDescription());

  return tooltip;
}

std::string ClientSpell::createEffectDescription() const {
  if (!_customDescription.empty()) return _customDescription;

  std::ostringstream oss;

  auto effectName = _effectName;
  auto effectArgs = _effectArgs;

  auto buffDuration = 0;
  auto statsBuff = false;
  const ClientBuffType *buff = nullptr;

  auto isBuff = _effectName == "buff" || _effectName == "debuff";
  if (isBuff) {
    auto buffName = effectArgs.s1;
    auto it = _client.gameData.buffTypes.find(buffName);
    assert(it != _client.gameData.buffTypes.end());
    buff = &it->second;

    buffDuration = buff->duration();

    if (!buff->effectName().empty()) {
      effectName = buff->effectName();
      effectArgs = buff->effectArgs();

      if (buff->tickTime() > 0) {
        auto numTicks = buff->duration() * 1000 / buff->tickTime();
        effectArgs.i1 *= numTicks;  // "format: n over m seconds"
      }
    } else {  // Assuming stats.  This will need to be changed when, for
              // example, stun debuffs are added
      statsBuff = true;  // In lieu of an effect name
    }
  }

  auto targetString = ""s;
  if (isBuff && buff->hasHitEffect())
    targetString = "attackers";
  else {
    if (_isAoE)
      targetString = "all targets within "s + toString(_range) + " podes"s;
    else {
      switch (_targetType) {
        case ALL:
          assert(false);
          targetString = "enemy or friendly target"s;
          break;
        case FRIENDLY:
          targetString = "friendly target"s;
          break;
        case ENEMY:
          targetString = "target"s;
          break;
        case SELF:
          targetString = "yourself"s;
          break;
        default:
          assert(false);
      }
    }
  }

  auto damageStr = _school.midSentenceString() + " damage"s;

  if (statsBuff)
    oss << buff->stats().buffDescription() << targetString;

  else if (effectName == "doDirectDamage")
    oss << "Deal " << effectArgs.i1 << " "s + damageStr + " to "
        << targetString;

  else if (effectName == "doDirectDamageWithModifiedThreat") {
    oss << "Deal " << effectArgs.i1 << " "s + damageStr + " to "
        << targetString;
    oss << ", while causing a " << (effectArgs.d1 < 1.0 ? "low" : "high")
        << " amount of threat";
  }

  else if (effectName == "heal")
    oss << "Restore " << effectArgs.i1 << " health to "s << targetString;

  else if (effectName == "dispellDebuff") {
    auto article = "a"s;
    if (effectArgs.s1 == "air" || effectArgs.s1 == "earth") article = "an"s;
    oss << "Dispell " << article << " " << effectArgs.s1 << " debuff from "s
        << targetString;
  }

  else if (effectName == "scaleThreat") {
    auto scalar = effectArgs.d1;
    if (scalar < 1.0)
      oss << "Reduce your threat against target by "
          << toInt((1.0 - scalar) * 100.0) << "%";
    else
      oss << "Increase your threat against " << targetString << " by "
          << toInt((scalar - 1.0) * 100.0) << "%";
  }

  else if (effectName == "randomTeleport") {
    oss << "Instantly teleport "s << effectArgs.i1
        << " podes in a random direction"s;
  }

  else if (effectName == "teleportToCity") {
    oss << "Instantly teleport to the Altar to Athena in your city"s;
  }

  if (isBuff) {
    if (buffDuration > 0) {
      auto conjunction = "for"s;
      if (buff->tickTime() > 0) conjunction = "over"s;
      oss << " " << conjunction << " " << sAsTimeDisplay(buffDuration);

    } else if (buff->cancelsOnOOE()) {
      oss << " until energy runs out"s;
    }
  }

  oss << ".";

  return oss.str();
}
