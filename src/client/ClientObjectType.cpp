#include <cassert>

#include <SDL.h>

#include "../Color.h"
#include "Client.h"
#include "ClientObjectType.h"
#include "SoundProfile.h"
#include "Surface.h"
#include "Tooltip.h"

ClientObjectType::ClientObjectType(const std::string &id)
    : SpriteType({}, id),
      _id(id),
      _canGather(false),
      _canDeconstruct(false),
      _containerSlots(0),
      _merchantSlots(0),
      _sounds(nullptr),
      _gatherParticles(nullptr),
      _transformTime(0) {}

const Tooltip &ClientObjectType::constructionTooltip() const {
  if (_constructionTooltip.hasValue()) return _constructionTooltip.value();

  const auto &client = *Client::_instance;

  _constructionTooltip = Tooltip{};
  auto &tooltip = _constructionTooltip.value();
  tooltip.setColor(Color::TOOLTIP_NAME);
  tooltip.addLine(_name);

  auto gapDrawn = false;
  if (canGather()) {
    if (!gapDrawn) {
      gapDrawn = true;
      tooltip.addGap();
    }
    std::string text = "Gatherable";
    if (!gatherReq().empty())
      text += " (requires " + client.tagName(gatherReq()) + ")";
    tooltip.addLine(text);
  }

  if (canDeconstruct()) {
    if (!gapDrawn) {
      gapDrawn = true;
      tooltip.addGap();
    }
    tooltip.addLine("Can pick up as item");
  }

  if (containerSlots() > 0) {
    if (!gapDrawn) {
      gapDrawn = true;
      tooltip.addGap();
    }
    tooltip.addLine("Container: " + toString(containerSlots()) + " slots");
  }

  if (merchantSlots() > 0) {
    if (!gapDrawn) {
      gapDrawn = true;
      tooltip.addGap();
    }
    tooltip.addLine("Merchant: " + toString(merchantSlots()) + " slots");
  }

  // Tags
  if (hasTags()) {
    tooltip.addGap();
    tooltip.setColor(Color::TODO);
    for (const std::string &tag : tags()) tooltip.addLine(client.tagName(tag));
  }

  tooltip.addGap();
  tooltip.setColor(Color::TODO);
  tooltip.addLine("Construction materials:");
  for (const auto &material : _materials) {
    const ClientItem &item = *dynamic_cast<const ClientItem *>(material.first);
    tooltip.addLine(makeArgs(material.second) + "x " + item.name());
  }

  if (!_constructionReq.empty()) {
    tooltip.addGap();
    tooltip.addLine("Requires tool: " + client.tagName(_constructionReq));
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
  corpseHighlightSurface.swapColors(Color::TODO, Color::TODO);
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
        ImageSet("Images/Objects/" + _id + "-construction.png");
}

ClientObjectType::ImageSet::ImageSet(const std::string &filename) {
  Surface surface(filename, Color::MAGENTA);
  normal = Texture(surface);
  surface.swapColors(Color::TODO, Color::TODO);
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
