#include "Tooltip.h"

#include <sstream>

#include "../HasTags.h"
#include "CRecipe.h"
#include "Client.h"
#include "ClientItem.h"
#include "Renderer.h"
#include "Tag.h"

extern Renderer renderer;

const px_t Tooltip::PADDING =
    4;  // Margins, and the height of gaps between lines.
TTF_Font *Tooltip::font = nullptr;
const px_t Tooltip::DEFAULT_MAX_WIDTH = 150;
const px_t Tooltip::NO_WRAP = 0;
std::unique_ptr<WordWrapper> Tooltip::wordWrapper;
ms_t Tooltip::timeThatTheLastRedrawWasOrdered{};
const Tooltip Tooltip::NO_TOOLTIP{};

Tooltip::Tooltip() {
  if (!font) font = TTF_OpenFont("AdvoCut.ttf", 10);
}

void Tooltip::setColor(const Color &color) { _color = color; }

void Tooltip::addLine(const std::string &line) {
  if (line == "") {
    addGap();
    return;
  }

  if (!wordWrapper) {
    wordWrapper =
        std::make_unique<WordWrapper>(WordWrapper(font, DEFAULT_MAX_WIDTH));
  }
  auto wrappedLines = wordWrapper->wrap(line);
  for (const auto &wrappedLine : wrappedLines)
    _content.push_back({font, wrappedLine, _color});
}

void Tooltip::addLines(const Lines &lines) {
  for (auto &line : lines) addLine(line);
}

void Tooltip::embed(const Tooltip &subTooltip) {
  subTooltip.generateIfNecessary();
  _content.push_back(subTooltip._generated);
}

void Tooltip::addItemGrid(const void *itemVector) {
  const auto &items = *reinterpret_cast<const ClientItem::vect_t *>(itemVector);

  static const size_t MAX_COLS = 5;
  static const auto GAP = 1_px;
  auto cols = min(items.size(), MAX_COLS);
  auto rows = (items.size() - 1) / MAX_COLS + 1;
  auto gridW = static_cast<px_t>(cols * (Client::ICON_SIZE + GAP) - GAP);
  auto gridH = static_cast<px_t>(rows * (Client::ICON_SIZE + GAP) - GAP);
  auto grid = Texture{gridW, gridH};

  auto xIndex = 0, yIndex = 0;
  renderer.pushRenderTarget(grid);
  for (auto slot : items) {
    auto x = xIndex * (Client::ICON_SIZE + GAP);
    auto y = yIndex * (Client::ICON_SIZE + GAP);

    // Background
    renderer.setDrawColor(Color::BLACK);
    renderer.fillRect({x, y, Client::ICON_SIZE, Client::ICON_SIZE});

    auto item = slot.first.type();

    // Quality border
    if (item) {
      const auto qualityColour = item->nameColor();
      if (qualityColour != Color::ITEM_QUALITY_COMMON) {
        item->client().images.itemQualityMask.setColourMod(qualityColour);
        item->client().images.itemQualityMask.draw(x + 1, y + 1);
      }

      // Icon
      item->icon().draw(x, y);
    }

    // Quantity
    auto qty = slot.second;
    if (qty > 1) {
      auto fg = Texture{font, toString(qty), Element::FONT_COLOR};
      auto bg = Texture{font, toString(qty), Color::UI_OUTLINE};
      auto topLeft = ScreenPoint{x + Client::ICON_SIZE - fg.width(),
                                 y + Client::ICON_SIZE - fg.height() + 2};
      for (auto i = -1; i <= 1; ++i)
        for (auto j = -1; j <= 1; ++j) {
          if (i == 0 && j == 0) continue;
          bg.draw(topLeft + ScreenPoint{i, j});
        }
      fg.draw(topLeft);
    }

    ++xIndex;
    if (xIndex >= MAX_COLS) {
      xIndex = 0;
      ++yIndex;
    }
  }
  renderer.popRenderTarget();

  grid.setBlend();
  _content.push_back(grid);
}

void Tooltip::addMerchantSlots(const std::vector<ClientMerchantSlot> &slots) {
  static const auto GAP = 1_px, ARROW_W = 30_px, QTY_W = 15_px,
                    PRICE_ICON_X = 0_px,
                    PRICE_QTY_X = PRICE_ICON_X + Client::ICON_SIZE,
                    WARE_ICON_X = PRICE_QTY_X + ARROW_W,
                    WARE_QTY_X = WARE_ICON_X + Client::ICON_SIZE + GAP,
                    TOTAL_W = WARE_QTY_X + QTY_W,
                    TEXT_Y = (Client::ICON_SIZE - Element::TEXT_HEIGHT) / 2 + 2;

  auto numActiveSlots = 0;
  for (const auto &slot : slots)
    if (slot.priceItem || slot.wareItem) ++numActiveSlots;

  auto textureHeight = numActiveSlots * (Client::ICON_SIZE + GAP) - GAP;
  auto texture = Texture{TOTAL_W, textureHeight};
  renderer.pushRenderTarget(texture);

  auto y = 0_px;
  for (const auto &slot : slots) {
    if (!slot.priceItem && !slot.wareItem) continue;

    if (slot.priceItem) slot.priceItem->icon().draw(PRICE_ICON_X, y);
    if (slot.wareItem) slot.wareItem->icon().draw(WARE_ICON_X, y);
    if (slot.priceQty > 1)
      Texture{font, "x"s + toString(slot.priceQty), Color::TOOLTIP_BODY}.draw(
          PRICE_QTY_X, y + TEXT_Y);
    if (slot.wareQty > 1)
      Texture{font, "x"s + toString(slot.wareQty), Color::TOOLTIP_BODY}.draw(
          WARE_QTY_X, y + TEXT_Y);

    static auto arrow = Texture{"Images/UI/arrow.png", Color::MAGENTA};
    arrow.draw((TOTAL_W - arrow.width()) / 2, y + TEXT_Y);

    y += Client::ICON_SIZE + GAP;
  }

  texture.setBlend();
  renderer.popRenderTarget();
  _content.push_back(texture);
}

void Tooltip::addItem(const ClientItem &item) {
  auto label = Texture{font, item.name(), item.nameColor()};

  auto GAP = 1_px;
  auto texture = Texture{Client::ICON_SIZE + 3 * GAP + label.width(),
                         Client::ICON_SIZE + 2 * GAP};
  renderer.pushRenderTarget(texture);

  item.icon().draw(GAP, GAP);
  auto labelX = Client::ICON_SIZE + 2 * GAP;
  auto labelY = (texture.height() - label.height()) / 2;
  label.draw(labelX, labelY);

  texture.setBlend();
  renderer.popRenderTarget();
  _content.push_back(texture);
}

void Tooltip::addTags(const HasTags &thingWithTags, const TagNames &tagNames) {
  if (!thingWithTags.hasTags()) return;

  addGap();
  setColor(Color::TOOLTIP_TAG);

  for (const auto &pair : thingWithTags.tags()) {
    const auto &tag = pair.first;
    addLine(tagNames[tag] + thingWithTags.toolSpeedDisplayText(tag));
  }
}

void Tooltip::addRecipe(const CRecipe &recipe, const TagNames &tagNames) {
  const auto *product = dynamic_cast<const ClientItem *>(recipe.product());
  if (product) {
    setColor(Color::TOOLTIP_BODY);
    auto oss = std::ostringstream{};
    oss << "Product";
    if (recipe.quantity() > 1) oss << " (x" << recipe.quantity() << ")";
    oss << ":";
    addLine(oss.str());

    embed(product->tooltip());
  }

  if (!recipe.materials().isEmpty()) {
    addGap();
    setColor(Color::TOOLTIP_BODY);
    addLine("Materials needed:");
    for (auto pair : recipe.materials()) {
      const auto *material = dynamic_cast<const ClientItem *>(pair.first);
      auto matText = " "s + material->name();
      if (pair.second > 1) matText += " ("s + toString(pair.second) + ")"s;
      setColor(material->nameColor());
      addLine(matText);
    }
  }
  if (!recipe.tools().empty()) {
    addGap();
    setColor(Color::TOOLTIP_BODY);
    addLine("Tools needed:");
    setColor(Color::TOOLTIP_TAG);
    for (auto toolID : recipe.tools()) {
      const auto tag = tagNames[toolID];
      addLine(" "s + tag);
    }
  }
}

px_t Tooltip::width() const {
  generateIfNecessary();
  return _generated.width();
}

px_t Tooltip::height() const {
  generateIfNecessary();
  return _generated.height();
}

void Tooltip::addGap() {
  if (_content.empty()) return;
  _content.push_back(Texture());
}

void Tooltip::draw(ScreenPoint p) const {
  generateIfNecessary();
  if (this != &NO_TOOLTIP) _generated.draw(p.x, p.y);
}

void Tooltip::forceAllToRedraw() {
  timeThatTheLastRedrawWasOrdered = SDL_GetTicks();
}

void Tooltip::generateIfNecessary() const {
  if (_generated && !isDueForARefresh()) return;

  generate();
  _timeGenerated = SDL_GetTicks();
}

void Tooltip::generate() const {
  // Calculate height and width of final tooltip
  px_t totalHeight = 2 * PADDING, totalWidth = 0;
  for (const Texture &item : _content) {
    if (item) {
      totalHeight += item.height();
      if (item.width() > totalWidth) totalWidth = item.width();
    } else {
      totalHeight += PADDING;
    }
  }
  totalWidth += 2 * PADDING;

  // Create background
  Texture background(totalWidth, totalHeight);
  renderer.pushRenderTarget(background);
  renderer.setDrawColor(Color::TOOLTIP_BACKGROUND);
  renderer.clear();
  background.setAlpha(0xdf);
  renderer.popRenderTarget();

  // Draw background
  _generated = Texture{totalWidth, totalHeight};
  renderer.pushRenderTarget(_generated);
  background.draw();

  // Draw border
  renderer.setDrawColor(Color::TOOLTIP_BORDER);
  renderer.drawRect({0, 0, totalWidth, totalHeight});

  // Draw text
  px_t y = PADDING;
  for (const Texture &item : _content) {
    if (!item)
      y += PADDING;
    else {
      item.draw(PADDING, y);
      y += item.height();
    }
  }

  _generated.setBlend(SDL_BLENDMODE_BLEND);
  renderer.popRenderTarget();
}

bool Tooltip::isDueForARefresh() const {
  return _timeGenerated < timeThatTheLastRedrawWasOrdered;
}

Tooltip Tooltip::basicTooltip(const std::string &text) {
  Tooltip tb;
  tb.addLine(text);
  return tb;
}
