#include "../Message.h"
#include "Client.h"

struct HotbarAction {
  HotbarAction() {}
  HotbarAction(HotbarCategory c, std::string s) : category(c), id(s) {}
  HotbarCategory category{HotbarCategory::HOTBAR_NONE};
  std::string id{};
  operator bool() const { return category != HotbarCategory::HOTBAR_NONE; }
};

std::vector<HotbarAction> actions;
using Icons = std::vector<Picture *>;
Icons icons;

static const auto HOTBAR_KEYS = std::map<SDL_Keycode, size_t>{
    {SDLK_BACKQUOTE, 0}, {SDLK_1, 1}, {SDLK_2, 2},  {SDLK_3, 3},
    {SDLK_4, 4},         {SDLK_5, 5}, {SDLK_6, 6},  {SDLK_7, 7},
    {SDLK_8, 8},         {SDLK_9, 9}, {SDLK_0, 10}, {SDLK_MINUS, 11},
    {SDLK_EQUALS, 12}};

static const int NO_BUTTON_BEING_ASSIGNED{-1};
static int buttonBeingAssigned{NO_BUTTON_BEING_ASSIGNED};

static void performAction(Client &client, int i) {
  if (actions[i].category == HotbarCategory::HOTBAR_SPELL)
    client.sendMessage({CL_CAST, actions[i].id});
  else if (actions[i].category == HotbarCategory::HOTBAR_RECIPE) {
    client.sendMessage({CL_CRAFT, actions[i].id});
    client.prepareAction("Crafting"s);
  }
}

void Client::initHotbar() {
  actions = std::vector<HotbarAction>(NUM_HOTBAR_BUTTONS, {});
  icons = Icons(NUM_HOTBAR_BUTTONS, nullptr);
  _hotbar = new Element({0, 0, NUM_HOTBAR_BUTTONS * 18, 18});
  _hotbar->setPosition((SCREEN_X - _hotbar->width()) / 2,
                       SCREEN_Y - _hotbar->height());

  for (auto i = 0; i != NUM_HOTBAR_BUTTONS; ++i) {
    auto button = new Button({i * 18, 0, 18, 18}, {},
                             [i, this]() { performAction(*this, i); });

    // Icons
    auto picture = new Picture({1, 1, ICON_SIZE, ICON_SIZE}, {});
    icons[i] = picture;
    button->addChild(picture);

    // Hotkey glyph
    static const auto HOTKEY_GLYPHS = std::vector<std::string>{
        "'", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="};
    button->addChild(new OutlinedLabel({0, -1, 15, 18}, HOTKEY_GLYPHS[i],
                                       Element::RIGHT_JUSTIFIED));

    // RMB: Assign
    button->setRightMouseUpFunction(
        [i, this](Element &e, const ScreenPoint &mousePos) {
          if (!collision(mousePos, {0, 0, e.rect().w, e.rect().h})) return;

          populateAssignerWindow();
          hotbar.assigner->show();
          buttonBeingAssigned = i;
        },
        nullptr);

    _hotbar->addChild(button);
    _hotbarButtons[i] = button;

    _hotbarCooldownLabels[i] =
        new OutlinedLabel{button->rect(), "", Element::CENTER_JUSTIFIED,
                          Element::CENTER_JUSTIFIED};
    _hotbar->addChild(_hotbarCooldownLabels[i]);
  }

  _hotbar->id("Hotbar");
  addUI(_hotbar);
  initAssignerWindow();
}

static void assignButton(Client &client, HotbarCategory category,
                         const std::string &id) {
  if (buttonBeingAssigned == NO_BUTTON_BEING_ASSIGNED) return;

  actions[buttonBeingAssigned] = {category, id};

  client.sendMessage(
      {CL_HOTBAR_BUTTON, makeArgs(buttonBeingAssigned, category, id)});

  client.hotbar.assigner->hide();
  buttonBeingAssigned = NO_BUTTON_BEING_ASSIGNED;
  client.refreshHotbar();
}

void Client::setHotbar(
    const std::vector<std::pair<int, std::string>> &buttons) {
  for (auto i = 0; i != buttons.size(); ++i) {
    auto category = HotbarCategory(buttons[i].first);
    buttonBeingAssigned = i;
    assignButton(*this, category, buttons[i].second);
  }
}

void Client::refreshHotbar() {
  for (auto i = 0; i != NUM_HOTBAR_BUTTONS; ++i) {
    if (!actions[i]) {
      _hotbarButtons[i]->setTooltip(
          "Right-click to assign an action to this button."s);
      _hotbarButtons[i]->disable();
      continue;
    }

    if (actions[i].category == HotbarCategory::HOTBAR_SPELL) {
      auto spellIt = gameData.spells.find(actions[i].id);
      if (spellIt == gameData.spells.end()) {
        _debug("Hotbar refers to invalid spell, " + actions[i].id,
               Color::CHAT_ERROR);
        continue;
      }

      _hotbarButtons[i]->enable();
      const auto &spell = *gameData.spells.find(actions[i].id)->second;

      icons[i]->changeTexture(spell.icon());

      _hotbarButtons[i]->setTooltip(spell.tooltip());

      auto spellIsKnown = _knownSpells.count(&spell) == 1;
      if (!spellIsKnown) _hotbarButtons[i]->disable();

    } else if (actions[i].category == HotbarCategory::HOTBAR_RECIPE) {
      auto it = gameData.recipes.find(actions[i].id);
      if (it == gameData.recipes.end()) {
        _debug("Hotbar refers to invalid recipe, " + actions[i].id,
               Color::CHAT_ERROR);
        continue;
      }

      _hotbarButtons[i]->enable();
      const auto &recipe = *it;

      const auto *product = dynamic_cast<const ClientItem *>(recipe.product());
      icons[i]->changeTexture(product->icon());

      auto tooltip = Tooltip{};
      tooltip.addLine("Craft recipe:"s);
      tooltip.addRecipe(recipe, gameData.tagNames);
      _hotbarButtons[i]->setTooltip(tooltip);

      auto recipeIsKnown = _knownRecipes.count(recipe.id()) == 1;
      if (!recipeIsKnown) _hotbarButtons[i]->disable();
    }
  }
  refreshHotbarCooldowns();
}

void Client::refreshHotbarCooldowns() {
  for (auto i = 0; i != NUM_HOTBAR_BUTTONS; ++i) {
    auto isSpell = actions[i].category == HotbarCategory::HOTBAR_SPELL;
    if (!isSpell) continue;

    auto cooldownIt = _spellCooldowns.find(actions[i].id);
    auto spellIsCoolingDown =
        cooldownIt != _spellCooldowns.end() && cooldownIt->second > 0;
    if (spellIsCoolingDown) {
      _hotbarButtons[i]->disable();
      _hotbarCooldownLabels[i]->changeText(
          msAsShortTimeDisplay(cooldownIt->second));
    } else {
      _hotbarButtons[i]->enable();
      _hotbarCooldownLabels[i]->changeText({});
    }
  }
}

static void onCategoryChange(Client &client) {
  client.hotbar.spellList->hide();
  client.hotbar.recipeList->hide();

  auto category = client.hotbar.categoryList->getSelected();

  if (category == "spell")
    client.hotbar.spellList->show();
  else if (category == "recipe")
    client.hotbar.recipeList->show();
}

void Client::initAssignerWindow() {
  static const auto LIST_WIDTH = 150_px, WIN_HEIGHT = 250_px,
                    CATEGORY_WIDTH = 50_px,
                    WIN_WIDTH = LIST_WIDTH + CATEGORY_WIDTH;
  hotbar.assigner = Window::WithRectAndTitle({0, 0, WIN_WIDTH, WIN_HEIGHT},
                                             "Assign action"s, mouse());

  // Center window above hotbar
  auto x = (SCREEN_X - hotbar.assigner->rect().w) / 2;
  auto y = _hotbar->rect().y - hotbar.assigner->rect().h;
  hotbar.assigner->rect({x, y, LIST_WIDTH, WIN_HEIGHT});

  // Category list
  hotbar.categoryList = new ChoiceList({0, 0, CATEGORY_WIDTH, WIN_HEIGHT},
                                       Element::TEXT_HEIGHT, *this);
  hotbar.assigner->addChild(hotbar.categoryList);
  hotbar.categoryList->onSelect = onCategoryChange;
  auto catLabel = new Label({}, " Spell");
  catLabel->id("spell");
  hotbar.categoryList->addChild(catLabel);
  catLabel = new Label({}, " Crafting");
  catLabel->id("recipe");
  hotbar.categoryList->addChild(catLabel);

  // Spell list
  hotbar.spellList = new List({CATEGORY_WIDTH, 0, LIST_WIDTH, WIN_HEIGHT},
                              Client::ICON_SIZE + 2);
  hotbar.assigner->addChild(hotbar.spellList);
  hotbar.spellList->hide();

  // Recipe list
  hotbar.recipeList = new List({CATEGORY_WIDTH, 0, LIST_WIDTH, WIN_HEIGHT},
                               Client::ICON_SIZE + 2);
  hotbar.assigner->addChild(hotbar.recipeList);
  hotbar.recipeList->hide();

  addWindow(hotbar.assigner);
}

void Client::populateAssignerWindow() {
  hotbar.spellList->clearChildren();
  for (const auto *spell : _knownSpells) {
    auto button = new Button({}, {}, [this, spell]() {
      assignButton(*this, HotbarCategory::HOTBAR_SPELL, spell->id());
    });
    button->addChild(new Picture(1, 1, spell->icon()));
    button->addChild(new Label({ICON_SIZE + 3, 0, 200, ICON_SIZE},
                               spell->name(), Element::LEFT_JUSTIFIED,
                               Element::CENTER_JUSTIFIED));
    button->id(spell->id());
    button->setTooltip(spell->tooltip());
    hotbar.spellList->addChild(button);
  }

  hotbar.recipeList->clearChildren();
  for (const auto &recipe : gameData.recipes) {
    auto recipeIsKnown = _knownRecipes.count(recipe.id()) == 1;
    if (!recipeIsKnown) continue;

    auto button = new Button({}, {}, [this, &recipe]() {
      assignButton(*this, HotbarCategory::HOTBAR_RECIPE, recipe.id());
    });
    const auto *product = dynamic_cast<const ClientItem *>(recipe.product());
    button->addChild(new Picture(1, 1, product->icon()));
    button->addChild(new Label({ICON_SIZE + 3, 0, 200, ICON_SIZE},
                               recipe.name(), Element::LEFT_JUSTIFIED,
                               Element::CENTER_JUSTIFIED));
    button->id(recipe.id());
    hotbar.recipeList->addChild(button);
  }
}

void Client::onHotbarKeyDown(SDL_Keycode key) {
  auto it = HOTBAR_KEYS.find(key);
  if (it == HOTBAR_KEYS.end()) return;
  auto index = it->second;

  if (!actions[index]) return;

  _hotbarButtons[index]->depress();
}

void Client::onHotbarKeyUp(SDL_Keycode key) {
  auto it = HOTBAR_KEYS.find(key);
  if (it == HOTBAR_KEYS.end()) return;
  auto index = it->second;

  if (!actions[index]) return;

  _hotbarButtons[index]->release(true);
}
