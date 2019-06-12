#include "ClientObjectType.h"

#include <SDL.h>

#include <cassert>

#include "../Color.h"
#include "Client.h"
#include "SoundProfile.h"
#include "Surface.h"
#include "Tooltip.h"
#include "Unlocks.h"

ClientObjectType::ClientObjectType(const std::string &id)
    : SpriteType({}, {}),
      _id(id),
      _canGather(false),
      _canDeconstruct(false),
      _containerSlots(0),
      _merchantSlots(0),
      _sounds(nullptr),
      _gatherParticles(nullptr),
      _transformTime(0) {}

bool ClientObjectType::canGather() const {
  if (!_canGather) return false;
  if (_exclusiveToQuest.empty()) return true;
  auto it = Client::instance().quests().find(_exclusiveToQuest);
  auto canGatherForQuest = it->second.state == CQuest::IN_PROGRESS;
  return canGatherForQuest;
}

const Tooltip &ClientObjectType::constructionTooltip() const {
  if (_constructionTooltip.hasValue()) return _constructionTooltip.value();

  const auto &client = *Client::_instance;

  _constructionTooltip = Tooltip{};
  auto &tooltip = _constructionTooltip.value();
  tooltip.setColor(Color::TOOLTIP_NAME);
  tooltip.addLine(_name);

  auto descriptionLines = std::vector<std::string>{};

  auto gapDrawn = false;

  if (isDebug()) descriptionLines.push_back("ID: "s + id());

  if (canGather()) {
    std::string text = "Gatherable";
    if (!gatherReq().empty())
      text += " (requires " + client.tagName(gatherReq()) + ")";
    descriptionLines.push_back(text);
  }

  if (canDeconstruct()) descriptionLines.push_back("Can pick up as item");

  if (containerSlots() > 0)
    descriptionLines.push_back("Container: " + toString(containerSlots()) +
                               " slots");

  if (merchantSlots() > 0)
    descriptionLines.push_back("Merchant: " + toString(merchantSlots()) +
                               " slots");

  if (!descriptionLines.empty()) tooltip.addGap();
  for (const auto &line : descriptionLines) tooltip.addLine(line);

  // Tags
  if (hasTags()) {
    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_TAG);
    for (const std::string &tag : tags()) tooltip.addLine(client.tagName(tag));
  }

  tooltip.addGap();
  tooltip.setColor(Color::TOOLTIP_BODY);
  tooltip.addLine("Construction materials:");
  for (const auto &material : _materials) {
    const ClientItem &item = *dynamic_cast<const ClientItem *>(material.first);
    tooltip.addLine(makeArgs(material.second) + "x " + item.name());
  }

  if (!_constructionReq.empty()) {
    tooltip.addGap();
    tooltip.addLine("Requires tool: " + client.tagName(_constructionReq));
  }

  // Unlocks
  auto unlockInfo = Unlocks::getEffectInfo({Unlocks::CONSTRUCT, id()});
  if (unlockInfo.hasEffect) {
    tooltip.addGap();
    tooltip.setColor(unlockInfo.color);
    tooltip.addLine(unlockInfo.message);
  }

  return tooltip;
}

const ClientObjectType::ImageSet &ClientObjectType::getProgressImage(
    ms_t timeRemaining) const {
  double progress = 1 - (1.0 * timeRemaining / _transformTime);
  size_t numFrames = _transformImages.size();
  int index = static_cast<int>(progress * (numFrames + 1)) - 1;
  if (_transformImages.empty()) index = -1;
  index = max<int>(index, -1);
  index = min<int>(index, _transformImages.size() -
                              1);  // Progress may be 100% due to server delay.
  if (index == -1) return _images;
  return _transformImages[index];
}

void ClientObjectType::corpseImage(const std::string &filename) {
  _corpseImage = Texture(filename, Color::MAGENTA);
  if (!_corpseImage) return;

  // Set corpse highlight image
  Surface corpseHighlightSurface(filename, Color::MAGENTA);
  if (!corpseHighlightSurface) return;
  corpseHighlightSurface.swapColors(Color::SPRITE_OUTLINE,
                                    Color::SPRITE_OUTLINE_HIGHLIGHT);
  _corpseHighlightImage = Texture(corpseHighlightSurface);
}

void ClientObjectType::addTransformImage(const std::string &filename) {
  _transformImages.push_back(ImageSet("Images/Objects/" + filename + ".png"));
}

void ClientObjectType::addMaterial(const ClientItem *item, size_t qty) {
  _materials.set(item, qty);
  auto isConstructionImageAlreadyInitialized = bool{_constructionImage.normal};
  if (!isConstructionImageAlreadyInitialized)
    _constructionImage =
        ImageSet("Images/Objects/" + _imageFile + "-construction.png");
}

ClientObjectType::ImageSet::ImageSet(const std::string &filename) {
  Surface surface(filename, Color::MAGENTA);
  normal = Texture(surface);
  surface.swapColors(Color::SPRITE_OUTLINE, Color::SPRITE_OUTLINE_HIGHLIGHT);
  highlight = Texture(surface);
}

void ClientObjectType::sounds(const std::string &id) {
  const Client &client = *Client::_instance;
  _sounds = client.findSoundProfile(id);
}

void ClientObjectType::calculateAndInitStrength() {
  auto isNPC = classTag() == 'n';
  if (isNPC) return;
  if (_strength.item == nullptr || _strength.quantity == 0)
    maxHealth(1);
  else
    maxHealth(_strength.item->strength() * _strength.quantity);
}
