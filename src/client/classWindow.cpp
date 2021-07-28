#include "../Message.h"
#include "Client.h"
#include "ui/Line.h"
#include "ui/OutlinedLabel.h"
#include "ui/ProgressBar.h"

void Client::initializeClassWindow() {
  const px_t SECTION_GAP = 6, MARGIN = 2, WIN_W = 216, WIN_H = 135, XP_H = 27,
             RESET_H = 15, TREES_Y = XP_H + SECTION_GAP;
  _classWindow =
      Window::WithRectAndTitle({80, 20, WIN_W, WIN_H}, "Class"s, mouse());
  const px_t TREES_H =
      _classWindow->contentHeight() - TREES_Y - RESET_H - MARGIN - SECTION_GAP;
  _talentTrees = new Element({0, TREES_Y, WIN_W, TREES_H});
  _classWindow->addChild(_talentTrees);

  _classWindow->addChild(new Line({0, XP_H + SECTION_GAP / 2 - 1}, WIN_W));

  // Class and level
  auto y = MARGIN;
  _levelLabel =
      new Label({MARGIN, MARGIN, WIN_W - 2 * MARGIN, Element::TEXT_HEIGHT}, {},
                Element::CENTER_JUSTIFIED);
  _classWindow->addChild(_levelLabel);
  y += Element::TEXT_HEIGHT + MARGIN;

  // XP
  const auto XP_RECT = ScreenRect{MARGIN, y, WIN_W - MARGIN * 2, 13};
  _classWindow->addChild(
      new ProgressBar<XP>(XP_RECT, _xp, _maxXP, Color::STAT_XP));
  _xpLabel = new Label(XP_RECT, {}, Element::CENTER_JUSTIFIED,
                       Element::CENTER_JUSTIFIED);
  _classWindow->addChild(_xpLabel);

  // Points available
  const px_t RESET_Y = TREES_Y + TREES_H + SECTION_GAP;
  _classWindow->addChild(new Line({0, RESET_Y - SECTION_GAP / 2 - 1}, WIN_W));
  _pointsAllocatedLabel =
      new Label({MARGIN, RESET_Y, WIN_W - 2 * MARGIN, RESET_H}, {});
  _classWindow->addChild(_pointsAllocatedLabel);

  // Reset button
  const auto RESET_BUTTON_W = 60_px;
  _classWindow->addChild(new Button(
      {WIN_W - MARGIN - RESET_BUTTON_W, RESET_Y, RESET_BUTTON_W, RESET_H},
      "Unlearn all"s, [this]() { confirmAndUnlearnTalents(); }));
}

void Client::confirmAndUnlearnTalents() {
  const std::string confirmationText =
      "Are you sure you want to unlearn all of your talents?";

  if (_unlearnTalentsConfirmationWindow)
    removeWindow(_unlearnTalentsConfirmationWindow);
  else
    _unlearnTalentsConfirmationWindow =
        new ConfirmationWindow(*this, confirmationText, CL_CLEAR_TALENTS, {});
  addWindow(_unlearnTalentsConfirmationWindow);
  _unlearnTalentsConfirmationWindow->show();
}

void Client::populateClassWindow() {
  if (!_character.getClass()) return;

  auto &classInfo = *_character.getClass();

  _levelLabel->changeText("Level "s + toString(_character.level()) + " "s +
                          classInfo.name());
  _xpLabel->changeText(toString(_xp) + "/"s + toString(_maxXP) +
                       " experience"s);

  auto pointsAllocated = totalTalentPointsAllocated();
  auto pointsAvailable = _character.level() - 1;
  _pointsAllocatedLabel->changeText("Talent points allocated: "s +
                                    toString(pointsAllocated) + "/"s +
                                    toString(pointsAvailable));

  _talentTrees->clearChildren();

  const px_t GAP = 10, TOTAL_GAP_WIDTH = GAP * (classInfo.trees().size()),
             TREE_WIDTH = (classInfo.trees().size() > 0
                               ? (_talentTrees->rect().w - TOTAL_GAP_WIDTH) /
                                     classInfo.trees().size()
                               : 0),
             TREE_HEIGHT = _talentTrees->rect().h;
  auto x = GAP / 2;
  auto treeElems = std::map<std::string, Element *>{};
  auto linesDrawn = size_t{0};
  for (auto &tree : classInfo.trees()) {
    auto treeElem = new Element({x, 0, TREE_WIDTH, 300});
    tree.element = treeElem;
    _talentTrees->addChild(treeElem);

    treeElem->addChild(new Label({0, 0, TREE_WIDTH, Element::TEXT_HEIGHT},
                                 tree.name, Element::CENTER_JUSTIFIED));
    auto y = Element::TEXT_HEIGHT;
    auto pointsInTreeText = toString(_pointsInTrees[tree.name]) + " points"s;
    auto pointsInTreeLabel =
        new Label({0, y, TREE_WIDTH, Element::TEXT_HEIGHT}, pointsInTreeText,
                  Element::CENTER_JUSTIFIED);
    pointsInTreeLabel->setColor(Color::WINDOW_FONT);
    treeElem->addChild(pointsInTreeLabel);

    x += TREE_WIDTH;

    if (linesDrawn < classInfo.trees().size() - 1)
      _talentTrees->addChild(
          new Line({x + (GAP / 2) - 1, 0}, TREE_HEIGHT, Element::VERTICAL));
    ++linesDrawn;

    x += GAP;
  }

  for (const auto &tree : classInfo.trees()) {
    auto baseY = 30_px;
    auto yDist = 0_px;
    if (tree.numTiers() > 1)
      yDist = (TREE_HEIGHT - 18 - baseY - GAP / 2) / (tree.numTiers() - 1);
    for (auto &tierPair : tree.talents) {
      auto tier = tierPair.first;
      auto y = static_cast<px_t>(baseY + tier * yDist);

      const auto &talents = tierPair.second;
      auto x = 1_px;
      auto xDist = 0_px;
      if (talents.size() > 1)
        xDist = (TREE_WIDTH - 18 - 2) / (talents.size() - 1);
      else
        x = (TREE_WIDTH - 18) / 2 + 1;

      for (const auto &talent : talents) {
        auto learnSpellButton = new Button(
            {x, y, 18, 18}, ""s,
            [this, &talent]() { this->sendMessage(talent.learnMessage); });
        learnSpellButton->setTooltip(talent.tooltip(*this));
        if (talent.icon)
          learnSpellButton->addChild(new Picture(1, 1, talent.icon));

        auto level = _talentLevels[talent.name];

        auto shouldDrawOutline = level > 0;
        if (shouldDrawOutline) {
          auto outlineRect =
              learnSpellButton->rect() + ScreenRect{-1, -1, 2, 2};
          tree.element->addChild(
              new ColorBlock(outlineRect, Color::UI_OUTLINE_HIGHLIGHT));
        }

        auto shouldShowLevel =
            (level > 0) && (talent.type == ClientTalent::STATS);
        if (shouldShowLevel) {
          learnSpellButton->addChild(new OutlinedLabel(
              {2, 2, ICON_SIZE - 1, ICON_SIZE - 1}, toString(level),
              Element::RIGHT_JUSTIFIED, Element::BOTTOM_JUSTIFIED));
        }

        tree.element->addChild(learnSpellButton);

        x += xDist;
      }

      y += yDist;
    }
  }

  onMouseMove();
}
