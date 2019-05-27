#include "Client.h"

static Window *assigner{nullptr};
static List *spellList{nullptr};
using Actions = std::vector<std::string>;
Actions actions;
using Icons = std::vector<Picture *>;
Icons icons;

static const auto HOTBAR_KEYS = std::map<SDL_Keycode, size_t>{
    {SDLK_BACKQUOTE, 0}, {SDLK_1, 1}, {SDLK_2, 2},  {SDLK_3, 3},
    {SDLK_4, 4},         {SDLK_5, 5}, {SDLK_6, 6},  {SDLK_7, 7},
    {SDLK_8, 8},         {SDLK_9, 9}, {SDLK_0, 10}, {SDLK_MINUS, 11},
    {SDLK_EQUALS, 12}};

static const int NO_BUTTON_BEING_ASSIGNED{-1};
static int buttonBeingAssigned{NO_BUTTON_BEING_ASSIGNED};

static void performAction(int i) {
  auto &client = Client::instance();
  if (!actions[i].empty()) client.sendMessage(CL_CAST, actions[i]);
}

void Client::initHotbar() {
  actions = Actions(NUM_HOTBAR_BUTTONS, {});
  icons = Icons(NUM_HOTBAR_BUTTONS, nullptr);
  _hotbar = new Element({0, 0, NUM_HOTBAR_BUTTONS * 18, 18});
  _hotbar->setPosition((SCREEN_X - _hotbar->width()) / 2,
                       SCREEN_Y - _hotbar->height());

  for (auto i = 0; i != NUM_HOTBAR_BUTTONS; ++i) {
    auto button =
        new Button({i * 18, 0, 18, 18}, {}, [i]() { performAction(i); });

    // Icons
    auto picture = new Picture({0, 0, ICON_SIZE, ICON_SIZE}, {});
    icons[i] = picture;
    button->addChild(picture);

    // Hotkey glyph
    static const auto HOTKEY_GLYPHS = std::vector<std::string>{
        "'", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="};
    button->addChild(new OutlinedLabel({0, -1, 15, 18}, HOTKEY_GLYPHS[i],
                                       Element::RIGHT_JUSTIFIED));

    // RMB: Assign
    button->setRightMouseUpFunction(
        [i](Element &e, const ScreenPoint &mousePos) {
          if (!collision(mousePos, {0, 0, e.rect().w, e.rect().h})) return;

          Client::instance().populateAssignerWindow();
          assigner->show();
          buttonBeingAssigned = i;
          Client::debug() << "Assigning button " << i << Log::endl;
        },
        nullptr);

    _hotbar->addChild(button);
    _hotbarButtons[i] = button;
  }

  addUI(_hotbar);
  initAssignerWindow();
}

void Client::refreshHotbar() {
  for (auto i = 0; i != NUM_HOTBAR_BUTTONS; ++i) {
    if (!actions[i].empty()) {
      _hotbarButtons[i]->enable();
      const auto &spell = *_spells.find(actions[i])->second;

      icons[i]->changeTexture(spell.icon());

      _hotbarButtons[i]->setTooltip(spell.tooltip());

      auto it = _spellCooldowns.find(actions[i]);
      auto spellIsCoolingDown = it != _spellCooldowns.end() && it->second > 0;
      if (spellIsCoolingDown) _hotbarButtons[i]->disable();

      auto spellIsKnown = _knownSpells.count(&spell) == 1;
      if (!spellIsKnown) _hotbarButtons[i]->disable();

    } else {
      _hotbarButtons[i]->setTooltip(
          "Right-click to assign an action to this button."s);
      _hotbarButtons[i]->disable();
    }
  }
}

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
    auto button = new Button({}, {}, [this, spell]() {
      if (buttonBeingAssigned == NO_BUTTON_BEING_ASSIGNED) return;
      actions[buttonBeingAssigned] = spell->id();
      assigner->hide();
      buttonBeingAssigned = NO_BUTTON_BEING_ASSIGNED;
      refreshHotbar();
    });
    button->addChild(new Picture(0, 0, spell->icon()));
    button->addChild(new Label({ICON_SIZE + 3, 0, 200, ICON_SIZE},
                               spell->name(), Element::LEFT_JUSTIFIED,
                               Element::CENTER_JUSTIFIED));
    button->id(spell->id());
    button->setTooltip(spell->tooltip());
    spellList->addChild(button);
  }
}

void Client::onHotbarKeyDown(SDL_Keycode key) {
  auto it = HOTBAR_KEYS.find(key);
  if (it == HOTBAR_KEYS.end()) return;
  auto index = it->second;

  _hotbarButtons[index]->depress();
}

void Client::onHotbarKeyUp(SDL_Keycode key) {
  auto it = HOTBAR_KEYS.find(key);
  if (it == HOTBAR_KEYS.end()) return;
  auto index = it->second;

  _hotbarButtons[index]->release(true);
}
