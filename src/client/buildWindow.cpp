#include <cassert>

#include "Client.h"
#include "Tooltip.h"
#include "ui/CheckBox.h"
#include "ui/Label.h"
#include "ui/Line.h"
#include "Unlocks.h"

extern Renderer renderer;

void Client::initializeBuildWindow() {
  static const px_t BUTTON_WIDTH = 100, BUTTON_HEIGHT = Element::TEXT_HEIGHT,
                    WIN_HEIGHT = toInt(10.5 * BUTTON_HEIGHT);

  _buildWindow = Window::WithRectAndTitle({80, 30, BUTTON_WIDTH, WIN_HEIGHT},
                                          "Construction", mouse());
  static const px_t GAP = 2;
  _buildWindow->addChild(
      new CheckBox(*this, {0, GAP, BUTTON_WIDTH, Element::TEXT_HEIGHT},
                   _multiBuild, "Build multiple"));
  px_t y = Element::TEXT_HEIGHT + GAP;
  _buildWindow->addChild(new Line({0, y}, BUTTON_WIDTH));
  y += GAP;
  _buildList = new ChoiceList({0, y, BUTTON_WIDTH, WIN_HEIGHT - y},
                              BUTTON_HEIGHT, *this);
  _buildWindow->addChild(_buildList);
}

void Client::populateBuildList() {
  assert(_buildList != nullptr);
  _buildList->clearChildren();
  for (const std::string &id : _knownConstructions) {
    auto ot = findObjectType(id);
    if (!ot) continue;

    Element *listElement = new Element({});
    _buildList->addChild(listElement);

    Label *label = new Label(
        {2, 0, listElement->width(), listElement->height()}, ot->name());
    listElement->addChild(label);
    auto unlockInfo = gameData.unlocks.getEffectInfo({Unlocks::CONSTRUCT, id});
    if (unlockInfo.hasEffect) label->setColor(unlockInfo.color);

    listElement->setLeftMouseUpFunction(chooseConstruction, listElement);
    listElement->setClient(*this);
    listElement->id(id);
    listElement->setTooltip(ot->constructionTooltip(*this));
  }
  _buildList->verifyBoxes();
  _buildList->markChanged();
}

void Client::chooseConstruction(Element &e, const ScreenPoint & /*mousePos*/) {
  auto &client = *e.client();
  const std::string selectedID = client._buildList->getSelected();
  if (selectedID.empty()) {
    client._selectedConstruction = nullptr;
    client._constructionFootprint = {};
  } else {
    auto ot = client.findObjectType(selectedID);
    client._selectedConstruction = ot;
    client._constructionFootprint = ot ? ot->image() : Texture{};
  }
}
