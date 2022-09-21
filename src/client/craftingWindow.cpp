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
  _craftingWindowFilters.push_back(new FilterRecipesByMaterial(*this));

  auto *const filterPane =
      new Element({FILTERS_PANE_X, CONTENT_Y, FILTERS_PANE_W, CONTENT_H});
  _craftingWindow->addChild(filterPane);
  auto *configurationPanel = new Element({0, 17, filterPane->width()});
  filterPane->addChild(configurationPanel);
  // Select filter
  auto &selectedFilter = _selectedCraftingWindowFilter;
  filterPane->addChild(new Button({0, 0, 50, 15}, "None",
                                  [&selectedFilter, configurationPanel]() {
                                    selectedFilter = nullptr;
                                    configurationPanel->clearChildren();
                                  }));
  const auto *matFilter = _craftingWindowFilters[0];
  filterPane->addChild(
      new Button({50, 0, 50, 15}, matFilter->buttonText(),
                 [&selectedFilter, matFilter, configurationPanel]() {
                   selectedFilter = matFilter;
                   configurationPanel->clearChildren();
                   matFilter->populateConfigurationPanel(*configurationPanel);
                 }));
  configurationPanel->height(filterPane->height() - 17);

  // Recipes
  Element *const recipesPane =
      new Element({RECIPES_PANE_X, CONTENT_Y, RECIPES_PANE_W, CONTENT_H});
  _craftingWindow->addChild(recipesPane);

  recipesPane->addChild(new Label({0, 0, RECIPES_PANE_W, HEADING_HEIGHT},
                                  "Recipes", Element::CENTER_JUSTIFIED));
  _recipeList = new ChoiceList(
      {0, HEADING_HEIGHT, RECIPES_PANE_W, CONTENT_H - HEADING_HEIGHT},
      ICON_SIZE + 2, *this);
  //_recipeList->doNotScrollToTopOnClear();
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
}

void Client::scrollRecipeListToTop() { _recipeList->scrollToTop(); }

void Client::indexRecipeInAllFilters(const CRecipe &recipe) {
  for (auto *filter : _craftingWindowFilters) filter->indexRecipe(recipe);
}

FilterRecipesByMaterial::FilterRecipesByMaterial(const Client &client)
    : m_client(client) {}

void FilterRecipesByMaterial::indexRecipe(const CRecipe &recipe) {
  for (const auto &matPair : recipe.materials()) {
    const ClientItem *material =
        dynamic_cast<const ClientItem *>(matPair.first);
    m_indexedRecipes.insert(std::make_pair(material, &recipe));
  }
}

CraftingWindowFilter::MatchingRecipes
FilterRecipesByMaterial::getMatchingRecipes() const {
  const auto selectedMatID = m_materialList->getSelected();
  if (selectedMatID.empty()) return {};
  const auto &items = m_client.gameData.items;
  auto it = items.find(selectedMatID);
  if (it == items.end()) return {};
  const auto *selectedMat = &it->second;

  auto recipes = MatchingRecipes{};
  auto startIt = m_indexedRecipes.lower_bound(selectedMat);
  auto endIt = m_indexedRecipes.upper_bound(selectedMat);
  for (auto it = startIt; it != endIt; ++it) recipes.insert(it->second);
  return recipes;
}

void FilterRecipesByMaterial::populateConfigurationPanel(Element &panel) const {
  auto matsByName = std::set<const ClientItem *, ClientItem::CompareName>{};
  for (const auto &pair : m_indexedRecipes) matsByName.insert(pair.first);

  m_materialList =
      new ChoiceList(panel.rect(), Client::ICON_SIZE, *panel.client());
  panel.addChild(m_materialList);
  for (const auto *material : matsByName) {
    auto *entry = new Element({});
    m_materialList->addChild(entry);
    entry->id(material->id());
    entry->addChild(new Picture(0, 0, material->icon()));
    entry->addChild(new Label(
        {Client::ICON_SIZE + 2, 0, entry->width(), entry->height()},
        material->name(), Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
  }
  m_materialList->verifyBoxes();
}
