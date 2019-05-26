#include "Client.h"

static Window *assigner{nullptr};
static List *spellList{nullptr};
using Icons = std::vector<const Texture *>;
static Icons hotbarIcons;

void Client::initHotbar() {
  hotbarIcons = Icons(NUM_HOTBAR_BUTTONS, nullptr);
  _hotbar = new Element({0, 0, NUM_HOTBAR_BUTTONS * 18, 18});
  _hotbar->setPosition((SCREEN_X - _hotbar->width()) / 2,
                       SCREEN_Y - _hotbar->height());

  for (auto i = 0; i != NUM_HOTBAR_BUTTONS; ++i) {
    auto button = new Button({i * 18, 0, 18, 18});
    button->setRightMouseUpFunction(
        [](Element &, const ScreenPoint &) {
          Client::instance().populateAssignerWindow();
          assigner->show();
        },
        nullptr);
    static const auto HOTKEY_GLYPHS = std::vector<std::string>{
        "'", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="};
    button->addChild(new OutlinedLabel({0, -1, 15, 18}, HOTKEY_GLYPHS[i],
                                       Element::RIGHT_JUSTIFIED));
    _hotbar->addChild(button);
    _hotbarButtons[i] = button;
  }

  addUI(_hotbar);
  initAssignerWindow();
}

void Client::refreshHotbar() {}

void Client::initAssignerWindow() {
  static const auto LIST_WIDTH = 150_px, LIST_HEIGHT = 250_px;
  assigner = Window::WithRectAndTitle({0, 0, LIST_WIDTH, LIST_HEIGHT},
                                      "Assign action"s);
  auto x = (SCREEN_X - assigner->rect().w) / 2;
  auto y = _hotbar->rect().y - assigner->rect().h;
  assigner->rect({x, y, LIST_WIDTH, LIST_HEIGHT});
  spellList = new List({0, 0, LIST_WIDTH, LIST_HEIGHT}, Client::ICON_SIZE + 2);
  assigner->addChild(spellList);

  addWindow(assigner);
}

void Client::populateAssignerWindow() {
  spellList->clearChildren();
  for (const auto *spell : _knownSpells) {
    auto button = new Button({}, spell->name());
    spellList->addChild(button);
  }
}
