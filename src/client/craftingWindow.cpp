#include <algorithm>
#include <cassert>

#include "Client.h"
#include "Renderer.h"
#include "Unlocks.h"
#include "craftingWindow.h"
#include "ui/Button.h"
#include "ui/CheckBox.h"
#include "ui/ColorBlock.h"
#include "ui/Element.h"
#include "ui/Label.h"
#include "ui/Line.h"
#include "ui/Picture.h"

extern Renderer renderer;

void Client::initializeCraftingWindow(Client &client) {
  client.initializeCraftingWindow();
}

void Client::initializeCraftingWindow() {
  // Set up crafting window
  static const px_t FILTERS_PANE_W = 150, RECIPES_PANE_W = 160,
                    DETAILS_PANE_W = 150, PANE_GAP = 6,
                    FILTERS_PANE_X = PANE_GAP / 2,
                    RECIPES_PANE_X = FILTERS_PANE_X + FILTERS_PANE_W + PANE_GAP,
                    DETAILS_PANE_X = RECIPES_PANE_X + RECIPES_PANE_W + PANE_GAP,
                    CRAFTING_WINDOW_W =
                        DETAILS_PANE_X + DETAILS_PANE_W + PANE_GAP / 2,

                    CONTENT_H = 200, CONTENT_Y = PANE_GAP / 2,
                    CRAFTING_WINDOW_H = CONTENT_Y + CONTENT_H + PANE_GAP / 2;

  _craftingWindow->rect({100, 50, CRAFTING_WINDOW_W, CRAFTING_WINDOW_H});
  _craftingWindow->setTitle("Crafting");
  _craftingWindow->addChild(new Line({RECIPES_PANE_X - PANE_GAP / 2, CONTENT_Y},
                                     CONTENT_H, Element::VERTICAL));
  _craftingWindow->addChild(new Line({DETAILS_PANE_X - PANE_GAP / 2, CONTENT_Y},
                                     CONTENT_H, Element::VERTICAL));

  // Filters
  auto *const filterPane =
      new Element({FILTERS_PANE_X, CONTENT_Y, FILTERS_PANE_W, CONTENT_H});
  _craftingWindow->addChild(filterPane);
  auto *configurationPanel = new Element;
  configurationPanel->setClient(*this);
  filterPane->addChild(configurationPanel);
  // Select-filter buttons
  const auto NUM_COLUMNS = 3;
  const auto BUTTON_W = FILTERS_PANE_W / NUM_COLUMNS, BUTTON_H = 14_px;
  auto y = 0_px;
  auto col = 0;
  auto &selectedFilter = _selectedCraftingWindowFilter;
  filterPane->addChild(new Button({BUTTON_W * col, y, BUTTON_W, BUTTON_H},
                                  "(Clear)",
                                  [&selectedFilter, configurationPanel]() {
                                    selectedFilter = nullptr;
                                    configurationPanel->clearChildren();
                                  }));
  for (const auto *filter : _craftingWindowFilters) {
    if (++col >= NUM_COLUMNS) {
      col = 0;
      y += BUTTON_H;
    }

    filterPane->addChild(new Button(
        {BUTTON_W * col, y, BUTTON_W, BUTTON_H}, filter->buttonText(),
        [&selectedFilter, filter, configurationPanel]() {
          selectedFilter = filter;
          configurationPanel->clearChildren();
          filter->populateConfigurationPanel(*configurationPanel);
        }));
  }
  y += BUTTON_H + 4;
  configurationPanel->rect(
      {0, y, filterPane->width(), filterPane->height() - y});

  // Recipes
  Element *const recipesPane =
      new Element({RECIPES_PANE_X, CONTENT_Y, RECIPES_PANE_W, CONTENT_H});
  _craftingWindow->addChild(recipesPane);

  recipesPane->addChild(new Label({0, 0, RECIPES_PANE_W, HEADING_HEIGHT},
                                  "Recipes", Element::CENTER_JUSTIFIED));
  _recipeList = new ChoiceList(
      {0, HEADING_HEIGHT, RECIPES_PANE_W, CONTENT_H - HEADING_HEIGHT},
      ICON_SIZE + 2, *this);
  _recipeList->doNotScrollToTopOnClear();
  recipesPane->addChild(_recipeList);
  // Click on a filter: force recipe list to refresh
  filterPane->setLeftMouseUpFunction(
      [](Element &e, const ScreenPoint &mousePos) {
        if (collision(mousePos, {0, 0, e.rect().w, e.rect().h}))
          e.markChanged();
      },
      _recipeList);
  // Repopulate recipe list before every refresh
  _recipeList->setPreRefreshFunction(populateRecipesList, _recipeList);
  _recipeList->setClient(*this);

  // Selected Recipe Details
  _detailsPane =
      new Element({DETAILS_PANE_X, CONTENT_Y, DETAILS_PANE_W, CONTENT_H});
  _craftingWindow->addChild(_detailsPane);
  refreshRecipeDetailsPane();  // Fill details pane initially
}

void Client::refreshRecipeDetailsPane() {
  Element &pane = *_detailsPane;
  pane.clearChildren();
  const ScreenRect &paneRect = pane.rect();

  // Close Button
  static const px_t BUTTON_HEIGHT = Element::TEXT_HEIGHT + 4, BUTTON_WIDTH = 40,
                    BUTTON_GAP = 3, BUTTON_Y = paneRect.h - BUTTON_HEIGHT;
  pane.addChild(new Button(
      {paneRect.w - BUTTON_WIDTH, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT},
      "Close", [this]() { Window::hideWindow(_craftingWindow); }));

  // If no recipe selected
  const std::string &selectedID = _recipeList->getSelected();
  if (selectedID == "") {
    _activeRecipe = nullptr;
    return;
  }

  // Crafting Button
  const auto WIDE_BUTTON_WIDTH = 50_px;
  auto x = BUTTON_GAP;
  pane.addChild(new Button({x, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT},
                           "Craft 1", [this]() { startCrafting(1); }));
  x += BUTTON_WIDTH + BUTTON_GAP;
  pane.addChild(new Button({x, BUTTON_Y, WIDE_BUTTON_WIDTH, BUTTON_HEIGHT},
                           "Craft max", [this]() { startCrafting(0); }));

  const std::set<CRecipe>::const_iterator it =
      Client::gameData.recipes.find(selectedID);
  if (it == Client::gameData.recipes.end()) {
    return;
  }
  const CRecipe &recipe = *it;
  _activeRecipe = &recipe;
  const ClientItem &product = *toClientItem(recipe.product());

  auto *details = new Scrollable({0, 0, pane.width(), BUTTON_Y - BUTTON_GAP});
  pane.addChild(details);

  auto y = 0_px;

  // Product tooltip
  auto *tooltip =
      new Picture({0, y, product.tooltip().width(), product.tooltip().height()},
                  product.tooltip().asTexture());
  details->addChild(tooltip);
  y += tooltip->height();

  // Quantity
  if (recipe.quantity() > 1) {
    details->addChild(
        new Label({0, y, pane.width(), Element::TEXT_HEIGHT},
                  "Quantity produced: "s + makeArgs(recipe.quantity())));
    y += HEADING_HEIGHT;
  }

  // Materials
  details->addChild(new Label({0, y, paneRect.w, Element::TEXT_HEIGHT},
                              "Required materials:"));
  y += Element::TEXT_HEIGHT;
  const auto H_GAP = 2_px;
  for (const auto &matCost : recipe.materials()) {
    if (!matCost.first) continue;
    const auto &mat = *toClientItem(matCost.first);
    const auto qty = matCost.second;
    auto entryText = mat.name();
    if (qty > 1) entryText += " (x"s + toString(qty) + ")"s;

    auto *icon = new Picture(0, y, mat.icon());
    auto name = new Label({ICON_SIZE + CheckBox::GAP, y, paneRect.w, ICON_SIZE},
                          entryText, Element::LEFT_JUSTIFIED,
                          Element::CENTER_JUSTIFIED);
    icon->setTooltip(mat.tooltip());
    name->setTooltip(mat.tooltip());
    details->addChild(icon);
    details->addChild(name);

    y += ICON_SIZE + H_GAP;
  }

  // Tools
  details->addChild(
      new Label({0, y, paneRect.w, Element::TEXT_HEIGHT}, "Required tools:"));
  y += Element::TEXT_HEIGHT;
  for (const auto &tool : recipe.tools()) {
    const auto toolName = gameData.tagNames[tool];
    const auto &iconImage = images.toolIcons[tool];
    const auto colour =
        _currentTools.hasTool(tool) ? Color::TOOL_PRESENT : Color::TOOL_MISSING;

    auto *icon = new Picture(0, y, iconImage);
    details->addChild(icon);
    auto *label =
        new Label({icon->width() + CheckBox::GAP, y, paneRect.w, icon->width()},
                  toolName, Label::LEFT_JUSTIFIED, Label::CENTER_JUSTIFIED);
    label->setColor(colour);
    details->addChild(label);

    y += ICON_SIZE + H_GAP;
  }

  // Unlocks
  const auto &unlockInfo =
      gameData.unlocks.getEffectInfo({Unlocks::CRAFT, recipe.id()});
  if (unlockInfo.hasEffect) {
    auto wordWrapper = WordWrapper{Element::font(), paneRect.w};
    const auto lines = wordWrapper.wrap(unlockInfo.message);
    for (const auto &line : lines) {
      auto l = new Label({0, y, paneRect.w, Element::TEXT_HEIGHT}, line);
      l->setColor(unlockInfo.color);
      details->addChild(l);
      y += Element::TEXT_HEIGHT;
    }
  }

  pane.markChanged();
}

void Client::onClickRecipe(Element &e, const ScreenPoint &mousePos) {
  // This intermediary function is necessary because UI mouse events don't
  // check for mouse collision, and so without it the details pane was redrawn
  // between any mouse down and mouse up.  This effectively disabled the
  // buttons in the pane.
  if (!collision(mousePos, {0, 0, e.rect().w, e.rect().h})) return;
  e.client()->refreshRecipeDetailsPane();
}

void Client::populateRecipesList(Element &e) {
  auto &recipesList = dynamic_cast<ChoiceList &>(e);
  auto &client = *recipesList.client();

  // TODO: get list from filter, sort that alphabetically, and use it.
  // Currently gets all known recipes, sorts, then checks against the filter.
  const auto *filter = client._selectedCraftingWindowFilter;
  const auto matchingRecipes = filter ? filter->getMatchingRecipes()
                                      : CraftingWindowFilter::MatchingRecipes{};

  recipesList.clearChildren();

  auto CompareName = [](const CRecipe *lhs, const CRecipe *rhs) {
    if (lhs->name() != rhs->name()) return lhs->name() < rhs->name();
    return lhs->id() < rhs->id();
  };
  auto knownRecipesSortedByName =
      std::set<const CRecipe *, decltype(CompareName)>{CompareName};
  const std::set<std::string> knownRecipes = client._knownRecipes;
  for (const CRecipe &recipe : client.gameData.recipes) {
    auto recipeIsKnown = knownRecipes.find(recipe.id()) != knownRecipes.end();
    if (!recipeIsKnown) continue;

    // Apply filter
    if (filter) {
      const auto recipeMatchesFilter = matchingRecipes.count(&recipe) > 0;
      if (!recipeMatchesFilter) continue;
    }

    knownRecipesSortedByName.insert(&recipe);
  }

  for (const auto *recipe : knownRecipesSortedByName) {
    const ClientItem &product = *toClientItem(recipe->product());
    Element *const recipeElement = new Element({});
    recipesList.addChild(recipeElement);
    recipeElement->addChild(
        new Picture({1, 1, ICON_SIZE, ICON_SIZE}, product.icon()));
    static const px_t NAME_X = ICON_SIZE + CheckBox::GAP + 1;

    auto name = new Label(
        {NAME_X, 0, recipeElement->rect().w - NAME_X, ICON_SIZE + 2},
        recipe->name(), Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED);
    recipeElement->addChild(name);
    auto unlockInfo =
        client.gameData.unlocks.getEffectInfo({Unlocks::CRAFT, recipe->id()});
    if (unlockInfo.hasEffect) name->setColor(unlockInfo.color);

    recipeElement->setLeftMouseUpFunction(onClickRecipe);
    recipeElement->setClient(*e.client());
    recipeElement->id(recipe->id());
  }
  const std::string oldSelectedRecipe = recipesList.getSelected();
  recipesList.verifyBoxes();
  if (recipesList.getSelected() != oldSelectedRecipe)
    client.refreshRecipeDetailsPane();

  // Make sure it isn't scrolled too far up
  if (recipesList.contentHeight() < recipesList.height())
    recipesList.scrollToTop();
  else if (recipesList.isScrolledPastBottom())
    recipesList.scrollToBottom();
}

void Client::scrollRecipeListToTop() { _recipeList->scrollToTop(); }

void Client::indexRecipeInAllFilters(const CRecipe &recipe) {
  for (auto *filter : _craftingWindowFilters) filter->indexRecipe(recipe);
}

void Client::createCraftingWindowFilters() {
  // Filters about the recipe
  _craftingWindowFilters.push_back(new SearchFilter());
  _craftingWindowFilters.push_back(new ReadyFilter(*this));
  _craftingWindowFilters.push_back(new MaterialFilter(*this));
  _craftingWindowFilters.push_back(new ToolFilter(*this));
  _craftingWindowFilters.push_back(_unlockFilter = new UnlockFilter(*this));

  _craftingWindowFilters.push_back(new CategoryFilter);

  // Filters about the product
  _craftingWindowFilters.push_back(new ProductTagFilter(*this));
  _craftingWindowFilters.push_back(new LvlReqFilter);
  _craftingWindowFilters.push_back(new QualityFilter);
  _craftingWindowFilters.push_back(new GearSlotFilter);
  _craftingWindowFilters.push_back(new StatsFilter);
}

// MaterialFilter

MaterialFilter::MaterialFilter(const Client &client) : m_client(client) {}

CraftingWindowFilter::MatchingRecipes MaterialFilter::getMatchingRecipes()
    const {
  const auto selectedMatID = m_list->getSelected();
  if (selectedMatID.empty()) return {};
  const auto &items = m_client.gameData.items;
  auto it = items.find(selectedMatID);
  if (it == items.end()) return {};
  const auto *selectedMat = &it->second;

  return recipesMatching(m_indexedRecipes, selectedMat);
}

void MaterialFilter::populateEntry(Element &entry, Key key) const {
  entry.addChild(new Picture(0, 0, key->icon()));
  entry.addChild(new Label(
      {Client::ICON_SIZE + 2, 0, entry.width(), entry.height()}, key->name(),
      Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
}

MaterialFilter::Keys MaterialFilter::getKeysFromRecipe(
    const CRecipe &recipe) const {
  auto materials = Keys{};
  for (const auto &pair : recipe.materials())
    materials.insert(dynamic_cast<const ClientItem *>(pair.first));
  return materials;
}

// ToolFilter

ToolFilter::ToolFilter(const Client &client) : m_client(client) {}

CraftingWindowFilter::MatchingRecipes ToolFilter::getMatchingRecipes() const {
  const auto selectedTool = m_list->getSelected();
  if (selectedTool.empty()) return {};

  return recipesMatching(m_indexedRecipes, selectedTool);
}

void ToolFilter::populateEntry(Element &entry, Key key) const {
  const auto toolName = m_client.gameData.tagNames[key];
  entry.addChild(new Picture(0, 0, m_client.images.toolIcons[key]));
  entry.addChild(
      new Label({Client::ICON_SIZE + 2, 0, entry.width(), entry.height()},
                toolName, Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
}

ToolFilter::Keys ToolFilter::getKeysFromRecipe(const CRecipe &recipe) const {
  return recipe.tools();
}

// Ready filter

ReadyFilter::ReadyFilter(Client &client) : m_client(client) {}

void ReadyFilter::indexRecipe(const CRecipe &recipe) {
  m_knownRecipes.insert(&recipe);
}

ReadyFilter::MatchingRecipes ReadyFilter::getMatchingRecipes() const {
  if (!m_requireAllTools && !m_requireAllMats) return {};

  auto recipesMatchingFilter = MatchingRecipes{};
  for (const auto *recipe : m_knownRecipes) {
    auto shouldExcludeThisRecipe = false;

    // All required materials
    if (m_requireAllMats) {
      for (const auto pair : recipe->materials())
        if (!m_client.playerHasItem(pair.first, pair.second)) {
          shouldExcludeThisRecipe = true;
          break;
        }
    }
    if (shouldExcludeThisRecipe) continue;

    // All required tools
    if (m_requireAllTools) {
      for (const auto tool : recipe->tools()) {
        if (!m_client.currentTools().hasTool(tool)) {
          shouldExcludeThisRecipe = true;
          break;
        }
      }
    }
    if (shouldExcludeThisRecipe) continue;

    recipesMatchingFilter.insert(recipe);
  }

  return recipesMatchingFilter;
}

void ReadyFilter::populateConfigurationPanel(Element &panel) const {
  auto *cb = new CheckBox(m_client, {0, 0, panel.width(), Element::TEXT_HEIGHT},
                          m_requireAllMats, "You have all materials");
  panel.addChild(cb);
  cb->onChange([](Client &client) {
    client.populateRecipesList();
    client.scrollRecipeListToTop();
  });

  cb = new CheckBox(m_client, {0, 13, panel.width(), Element::TEXT_HEIGHT},
                    m_requireAllTools, "You have all tools");
  panel.addChild(cb);
  cb->onChange([](Client &client) {
    client.populateRecipesList();
    client.scrollRecipeListToTop();
  });
}

// Search filter

void SearchFilter::indexRecipe(const CRecipe &recipe) {
  const auto &product = *dynamic_cast<const ClientItem *>(recipe.product());
  auto searchableText = product.name() + "_"s + recipe.name();
  std::transform(searchableText.begin(), searchableText.end(),
                 searchableText.begin(), ::tolower);
  m_indexedRecipes[&recipe] = searchableText;
}

CraftingWindowFilter::MatchingRecipes SearchFilter::getMatchingRecipes() const {
  auto searchText = m_searchBox->text();
  if (searchText.empty()) return {};
  std::transform(searchText.begin(), searchText.end(), searchText.begin(),
                 ::tolower);

  auto recipes = MatchingRecipes{};
  for (const auto &pair : m_indexedRecipes) {
    const auto names = pair.second;
    const auto recipeMatches = names.find(searchText) != std::string::npos;
    if (recipeMatches) recipes.insert(pair.first);
  }

  return recipes;
}

void SearchFilter::populateConfigurationPanel(Element &panel) const {
  const auto GAP = 2_px, LABEL_W = 60_px,
             TEXT_W = panel.width() - LABEL_W - GAP, ROW_H = 13_px;
  panel.addChild(new Label({GAP, GAP, LABEL_W, ROW_H}, "Search text:"));
  panel.addChild(m_searchBox = new TextBox(
                     *panel.client(), {LABEL_W + GAP, GAP, TEXT_W, ROW_H}));
  m_searchBox->setOnChange(
      [](void *pClient) {
        auto &client = *reinterpret_cast<Client *>(pClient);
        client.populateRecipesList();
      },
      panel.client());
}

// LvlReqFilter

void LvlReqFilter::indexRecipe(const CRecipe &recipe) {
  const auto &product = *recipe.product();
  if (product.lvlReq() == 0) return;
  if (!product.isGear()) return;

  m_indexedRecipes.insert(std::make_pair(product.lvlReq(), &recipe));
}

CraftingWindowFilter::MatchingRecipes LvlReqFilter::getMatchingRecipes() const {
  Level minLevel = m_minLevel->hasText()
                       ? static_cast<Level>(m_minLevel->textAsNum())
                       : -1000;
  Level maxLevel = m_maxLevel->hasText()
                       ? static_cast<Level>(m_maxLevel->textAsNum())
                       : +1000;
  if (minLevel > maxLevel) return {};

  auto recipes = MatchingRecipes{};
  auto startIt = m_indexedRecipes.lower_bound(minLevel);
  auto endIt = m_indexedRecipes.upper_bound(maxLevel);
  for (auto it = startIt; it != endIt; ++it) recipes.insert(it->second);
  return recipes;
}

void LvlReqFilter::populateConfigurationPanel(Element &panel) const {
  const auto GAP = 2_px, LABEL_W = 100_px, TEXT_W = 40_px, ROW_H = 13_px;
  panel.addChild(new Label({GAP, GAP, LABEL_W, ROW_H}, "Min level required"));
  panel.addChild(
      new Label({GAP, ROW_H + GAP, LABEL_W, ROW_H}, "Max level required"));
  m_minLevel = new TextBox(*panel.client(), {LABEL_W + GAP, GAP, TEXT_W, ROW_H},
                           TextBox::NUMERALS);
  m_maxLevel =
      new TextBox(*panel.client(), {LABEL_W + GAP, ROW_H + GAP, TEXT_W, ROW_H},
                  TextBox::NUMERALS);
  panel.addChild(m_minLevel);
  panel.addChild(m_maxLevel);
  m_minLevel->setOnChange(
      [](void *pClient) {
        auto &client = *reinterpret_cast<Client *>(pClient);
        client.populateRecipesList();
      },
      panel.client());
  m_maxLevel->setOnChange(
      [](void *pClient) {
        auto &client = *reinterpret_cast<Client *>(pClient);
        client.populateRecipesList();
      },
      panel.client());
}

// CategoryFilter

CraftingWindowFilter::MatchingRecipes CategoryFilter::getMatchingRecipes()
    const {
  const auto selectedCategory = m_list->getSelected();
  if (selectedCategory.empty()) return {};

  return recipesMatching(m_indexedRecipes, selectedCategory);
}

void CategoryFilter::populateEntry(Element &entry, Key key) const {
  entry.addChild(new Label(entry.rectToFill(), " "s + key));
}

CategoryFilter::Keys CategoryFilter::getKeysFromRecipe(
    const CRecipe &recipe) const {
  if (recipe.category().empty())
    return {"(Uncategorised)"s};
  else
    return {recipe.category()};
}

// QualityFilter

CraftingWindowFilter::MatchingRecipes QualityFilter::getMatchingRecipes()
    const {
  const auto qualityString = m_list->getSelected();
  if (qualityString.empty()) return {};
  const auto selectedQuality =
      static_cast<Item::Quality>(std::stoi(qualityString));

  return recipesMatching(m_indexedRecipes, selectedQuality);
}

void QualityFilter::populateEntry(Element &entry, Key key) const {
  auto *label =
      new Label(entry.rectToFill(), " "s + ClientItem::qualityName(key));
  label->setColor(ClientItem::qualityColor(key));
  entry.addChild(label);
}

QualityFilter::Keys QualityFilter::getKeysFromRecipe(
    const CRecipe &recipe) const {
  return {recipe.product()->quality()};
}

// Gear-slot filter

CraftingWindowFilter::MatchingRecipes GearSlotFilter::getMatchingRecipes()
    const {
  const auto slotString = m_list->getSelected();
  if (slotString.empty()) return {};
  const auto selectedSlot = static_cast<Item::GearSlot>(std::stoi(slotString));

  return recipesMatching(m_indexedRecipes, selectedSlot);
}

void GearSlotFilter::populateEntry(Element &entry, Key key) const {
  entry.addChild(
      new Label(entry.rectToFill(), " "s + Client::GEAR_SLOT_NAMES[key]));
}

GearSlotFilter::Keys GearSlotFilter::getKeysFromRecipe(
    const CRecipe &recipe) const {
  auto gearSlot = recipe.product()->gearSlot();
  if (gearSlot == Item::NOT_GEAR)
    return {};
  else
    return {gearSlot};
}

// Product-tag filter

ProductTagFilter::ProductTagFilter(const Client &client) : m_client(client) {}

CraftingWindowFilter::MatchingRecipes ProductTagFilter::getMatchingRecipes()
    const {
  const auto selectedTag = m_list->getSelected();
  if (selectedTag.empty()) return {};

  return recipesMatching(m_indexedRecipes, selectedTag);
}

void ProductTagFilter::populateEntry(Element &entry, Key key) const {
  const auto tagName = m_client.gameData.tagNames[key];
  const auto &icon = m_client.images.toolIcons[key];
  if (MemoisedImageDirectory::doesTextureExistInDirectory(icon))
    entry.addChild(new Picture(0, 0, icon));
  entry.addChild(
      new Label({Client::ICON_SIZE + 2, 0, entry.width(), entry.height()},
                tagName, Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
}

ProductTagFilter::Keys ProductTagFilter::getKeysFromRecipe(
    const CRecipe &recipe) const {
  auto keys = Keys{};
  for (const auto &pair : recipe.product()->tags()) keys.insert(pair.first);
  return keys;
}

// Unlock-chance filter

UnlockFilter::UnlockFilter(const Client &client) : m_client(client) {}

void UnlockFilter::onUnlockChancesChanged(
    const std::set<std::string> &knownRecipes) {
  m_indexedRecipes.clear();
  for (const auto &recipe : m_client.gameData.recipes) {
    const auto recipeIsKnown = knownRecipes.count(recipe.id()) > 0;
    if (recipeIsKnown) indexRecipe(recipe);
  }
}

CraftingWindowFilter::MatchingRecipes UnlockFilter::getMatchingRecipes() const {
  const auto chanceID = m_list->getSelected();
  if (chanceID.empty()) return {};
  const auto selectedChance =
      static_cast<Unlocks::UnlockChance>(std::stoi(chanceID));

  return recipesMatching(m_indexedRecipes, selectedChance);
}

void UnlockFilter::populateEntry(Element &entry, Key key) const {
  const auto text = " "s + Unlocks::chanceName(key) + " chance"s;
  auto *l = new Label(entry.rectToFill(), text);
  l->setColor(Unlocks::chanceColor(key));
  entry.addChild(l);
}

UnlockFilter::Keys UnlockFilter::getKeysFromRecipe(
    const CRecipe &recipe) const {
  const auto &unlockInfo =
      m_client.gameData.unlocks.getEffectInfo({Unlocks::CRAFT, recipe.id()});
  if (!unlockInfo.hasEffect) return {};
  return {unlockInfo.chance};
}

// Stats filter

CraftingWindowFilter::MatchingRecipes StatsFilter::getMatchingRecipes() const {
  const auto selectedTag = m_list->getSelected();
  if (selectedTag.empty()) return {};

  return recipesMatching(m_indexedRecipes, selectedTag);
}

void StatsFilter::populateEntry(Element &entry, Key key) const {
  entry.addChild(new Label(entry.rectToFill(), " "s + key));
}

StatsFilter::Keys StatsFilter::getKeysFromRecipe(const CRecipe &recipe) const {
  return recipe.product()->stats().namesOfIncludedStats();
}
