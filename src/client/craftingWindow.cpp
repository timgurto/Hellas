#include <cassert>

#include "Client.h"
#include "Renderer.h"
#include "ui/Button.h"
#include "ui/CheckBox.h"
#include "ui/ColorBlock.h"
#include "ui/Element.h"
#include "ui/Label.h"
#include "ui/Line.h"

extern Renderer renderer;

void Client::initializeCraftingWindow(Client &client) {
  client.initializeCraftingWindow();
}

void Client::initializeCraftingWindow() {
  // For crafting filters
  for (const Recipe &recipe : _recipes) {
    for (auto matPair : recipe.materials())
      _matFilters[toClientItem(matPair.first)] = false;
    for (const std::string &tagName : recipe.product()->tags())
      _tagFilters[tagName] = false;
  }
  _haveMatsFilter = false;
  _tagOr = _matOr = false;
  _haveToolsFilter = true;
  _tagFilterSelected = _matFilterSelected = false;

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
  _craftingWindow->addChild(new Line(RECIPES_PANE_X - PANE_GAP / 2, CONTENT_Y,
                                     CONTENT_H, Element::VERTICAL));
  _craftingWindow->addChild(new Line(DETAILS_PANE_X - PANE_GAP / 2, CONTENT_Y,
                                     CONTENT_H, Element::VERTICAL));

  // Filters
  Element *const filterPane =
      new Element({FILTERS_PANE_X, CONTENT_Y, FILTERS_PANE_W, CONTENT_H});
  _craftingWindow->addChild(filterPane);
  filterPane->addChild(new Label({0, 0, FILTERS_PANE_W, HEADING_HEIGHT},
                                 "Filters", Element::CENTER_JUSTIFIED));
  px_t y = HEADING_HEIGHT;
  CheckBox *pCB = new CheckBox({0, y, FILTERS_PANE_W, Element::TEXT_HEIGHT},
                               _haveMatsFilter, "Have all materials");
  pCB->setTooltip("Only show recipes for which you have the materials");
  filterPane->addChild(pCB);
  y += Element::TEXT_HEIGHT;
  filterPane->addChild(new Line(0, y + LINE_GAP / 2, FILTERS_PANE_W));
  y += LINE_GAP;

  static const px_t TOTAL_FILTERS_HEIGHT =
                        CONTENT_H - y - 4 * Element::TEXT_HEIGHT - LINE_GAP,
                    // TOTAL_FILTERS_HEIGHT = CRAFTING_WINDOW_H - PANE_GAP/2 - y
                    // - 4 * Element::TEXT_HEIGHT - LINE_GAP,
      TAG_LIST_HEIGHT = TOTAL_FILTERS_HEIGHT / 2,
                    MATERIALS_LIST_HEIGHT =
                        TOTAL_FILTERS_HEIGHT - TAG_LIST_HEIGHT;

  // Tag filters
  filterPane->addChild(
      new Label({0, y, FILTERS_PANE_W, Element::TEXT_HEIGHT}, "Item tag:"));
  y += Element::TEXT_HEIGHT;
  _tagList =
      new List({0, y, FILTERS_PANE_W, TAG_LIST_HEIGHT}, Element::TEXT_HEIGHT);
  filterPane->addChild(_tagList);
  y += TAG_LIST_HEIGHT;

  pCB = new CheckBox({0, y, FILTERS_PANE_W / 4, Element::TEXT_HEIGHT}, _tagOr,
                     "Any");
  pCB->setTooltip(
      "Only show recipes whose product has at least one of the selected tags.");
  filterPane->addChild(pCB);

  pCB = new CheckBox(
      {FILTERS_PANE_W / 2, y, FILTERS_PANE_W / 4, Element::TEXT_HEIGHT}, _tagOr,
      "All", true);
  pCB->setTooltip(
      "Only show recipes whose product has all of the selected tags.");
  filterPane->addChild(pCB);

  y += Element::TEXT_HEIGHT;
  filterPane->addChild(new Line(0, y + LINE_GAP / 2, FILTERS_PANE_W));
  y += LINE_GAP;

  // Material filters
  filterPane->addChild(
      new Label({0, y, FILTERS_PANE_W, Element::TEXT_HEIGHT}, "Materials:"));
  y += Element::TEXT_HEIGHT;
  _materialsList =
      new List({0, y, FILTERS_PANE_W, MATERIALS_LIST_HEIGHT}, ICON_SIZE);
  filterPane->addChild(_materialsList);
  y += MATERIALS_LIST_HEIGHT;
  pCB = new CheckBox({0, y, FILTERS_PANE_W / 4, Element::TEXT_HEIGHT}, _matOr,
                     "Any");
  pCB->setTooltip(
      "Only show recipes that require at least one of the selected materials.");
  filterPane->addChild(pCB);

  pCB = new CheckBox(
      {FILTERS_PANE_W / 2, y, FILTERS_PANE_W / 4, Element::TEXT_HEIGHT}, _matOr,
      "All", true);
  pCB->setTooltip(
      "Only show recipes that require all of the selected materials.");
  filterPane->addChild(pCB);

  populateFilters();

  // Recipes
  Element *const recipesPane =
      new Element({RECIPES_PANE_X, CONTENT_Y, RECIPES_PANE_W, CONTENT_H});
  _craftingWindow->addChild(recipesPane);

  recipesPane->addChild(new Label({0, 0, RECIPES_PANE_W, HEADING_HEIGHT},
                                  "Recipes", Element::CENTER_JUSTIFIED));
  _recipeList = new ChoiceList(
      {0, HEADING_HEIGHT, RECIPES_PANE_W, CONTENT_H - HEADING_HEIGHT},
      ICON_SIZE + 2);
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

  // Selected Recipe Details
  _detailsPane =
      new Element({DETAILS_PANE_X, CONTENT_Y, DETAILS_PANE_W, CONTENT_H});
  _craftingWindow->addChild(_detailsPane);
  selectRecipe(*_detailsPane, {});  // Fill details pane initially
}

void Client::selectRecipe(Element &e, const ScreenPoint &mousePos) {
  auto &client = *_instance;

  if (!collision(mousePos, {0, 0, e.rect().w, e.rect().h})) return;
  Element &pane = *client._detailsPane;
  pane.clearChildren();
  const ScreenRect &paneRect = pane.rect();

  // Close Button
  static const px_t BUTTON_HEIGHT = Element::TEXT_HEIGHT + 4, BUTTON_WIDTH = 40,
                    BUTTON_GAP = 3, BUTTON_Y = paneRect.h - BUTTON_HEIGHT;
  pane.addChild(new Button(
      {paneRect.w - BUTTON_WIDTH, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT},
      "Close", [&client]() { Window::hideWindow(client._craftingWindow); }));

  // If no recipe selected
  const std::string &selectedID = client._recipeList->getSelected();
  if (selectedID == "") {
    client._activeRecipe = nullptr;
    return;
  }

  // Crafting Button
  pane.addChild(new Button({paneRect.w - 2 * BUTTON_WIDTH - BUTTON_GAP,
                            BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT},
                           "Craft", startCrafting));

  const std::set<Recipe>::const_iterator it =
      _instance->_recipes.find(selectedID);
  if (it == _instance->_recipes.end()) {
    return;
  }
  const Recipe &recipe = *it;
  _instance->_activeRecipe = &recipe;
  const ClientItem &product = *toClientItem(recipe.product());

  // Title
  std::string titleText = toClientItem(recipe.product())->name();
  if (recipe.quantity() > 1)
    titleText += " (x" + makeArgs(recipe.quantity()) + ")";
  pane.addChild(new Label({0, 0, paneRect.w, HEADING_HEIGHT}, titleText,
                          Element::CENTER_JUSTIFIED));
  px_t y = HEADING_HEIGHT;

  // Icon
  auto icon = new Picture({0, y, ICON_SIZE, ICON_SIZE}, product.icon());
  icon->setTooltip(product.tooltip());
  pane.addChild(icon);
  y += ICON_SIZE;

  // Divider
  pane.addChild(new Line(0, y + LINE_GAP / 2, paneRect.w));
  y += LINE_GAP;

  // Materials list
  const px_t TOTAL_LIST_SPACE =
                 BUTTON_Y - BUTTON_GAP - y - 2 * Element::TEXT_HEIGHT,
             MATS_LIST_HEIGHT = TOTAL_LIST_SPACE / 2,
             TOOLS_LIST_HEIGHT = TOTAL_LIST_SPACE - MATS_LIST_HEIGHT;
  pane.addChild(new Label({0, y, paneRect.w, Element::TEXT_HEIGHT},
                          "Required materials:"));
  y += Element::TEXT_HEIGHT;
  List *const matsList =
      new List({0, y, paneRect.w, MATS_LIST_HEIGHT}, ICON_SIZE);
  y += MATS_LIST_HEIGHT;
  pane.addChild(matsList);
  for (const std::pair<const Item *, size_t> &matCost : recipe.materials()) {
    assert(matCost.first);
    const ClientItem &mat = *toClientItem(matCost.first);
    const size_t qty = matCost.second;
    std::string entryText = mat.name();
    if (qty > 1) entryText += " x" + toString(qty);
    Element *const entry = new Element({0, 0, paneRect.w, ICON_SIZE});
    entry->setTooltip(mat.tooltip());
    matsList->addChild(entry);
    entry->addChild(new Picture({0, 0, ICON_SIZE, ICON_SIZE}, mat.icon()));
    entry->addChild(new Label(
        {ICON_SIZE + CheckBox::GAP, 0, paneRect.w, ICON_SIZE}, entryText,
        Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
  }
  pane.addChild(
      new Label({0, y, paneRect.w, Element::TEXT_HEIGHT}, "Required tools:"));
  y += Element::TEXT_HEIGHT;
  List *const toolsList = new List({5, y, paneRect.w, TOOLS_LIST_HEIGHT});
  pane.addChild(toolsList);
  for (const std::string &tool : recipe.tools()) {
    static const px_t TOOL_MARGIN = 5;
    Label *entry = new Label(
        {TOOL_MARGIN, 0, paneRect.w - TOOL_MARGIN, Element::TEXT_HEIGHT},
        client.tagName(tool));
    toolsList->addChild(entry);
  }
  pane.markChanged();
}

void Client::populateFilters() {
  const auto &client = *Client::_instance;

  const px_t FILTERS_PANE_W = _materialsList->parent()->width();

  // Restrict shown filters to known recipes
  std::set<std::string> knownTags;
  std::set<const ClientItem *> knownMats;
  for (const Recipe &recipe : _recipes)
    if (_knownRecipes.find(recipe.id()) !=
        _knownRecipes.end()) {  // user knows this recipe
      for (const std::string &tag : recipe.product()->tags())
        knownTags.insert(tag);
      for (const auto &matPair : recipe.materials())
        knownMats.insert(dynamic_cast<const ClientItem *>(matPair.first));
    }

  // Tags
  _tagList->clearChildren();
  for (auto &pair : _tagFilters) {
    if (knownTags.find(pair.first) ==
        knownTags.end())  // User doesn't know about this tag yet.
      continue;
    _tagList->addChild(
        new CheckBox({0, 0, FILTERS_PANE_W, Element::TEXT_HEIGHT}, pair.second,
                     client.tagName(pair.first)));
  }

  // Materials
  _materialsList->clearChildren();
  for (auto &pair : _matFilters) {
    if (knownMats.find(pair.first) == knownMats.end()) continue;
    CheckBox *const mat = new CheckBox({}, pair.second);
    static const px_t ICON_X = CheckBox::BOX_SIZE + CheckBox::GAP,
                      LABEL_X = ICON_X + ICON_SIZE + CheckBox::GAP,
                      LABEL_W = FILTERS_PANE_W - LABEL_X;
    mat->addChild(
        new Picture({ICON_X, 0, ICON_SIZE, ICON_SIZE}, pair.first->icon()));
    mat->addChild(new Label({LABEL_X, 0, LABEL_W, ICON_SIZE},
                            pair.first->name(), Element::LEFT_JUSTIFIED,
                            Element::CENTER_JUSTIFIED));
    _materialsList->addChild(mat);
  }
}

void Client::populateRecipesList(Element &e) {
  // Check which filters are applied
  _instance->_matFilterSelected = false;
  for (const std::pair<const ClientItem *, bool> &filter :
       _instance->_matFilters) {
    if (filter.second) {
      _instance->_matFilterSelected = true;
      break;
    }
  }
  _instance->_tagFilterSelected = false;
  for (const std::pair<std::string, bool> &filter : _instance->_tagFilters) {
    if (filter.second) {
      _instance->_tagFilterSelected = true;
      break;
    }
  }

  ChoiceList &recipesList = dynamic_cast<ChoiceList &>(e);
  recipesList.clearChildren();

  const std::set<std::string> knownRecipes = Client::_instance->_knownRecipes;
  for (const Recipe &recipe : _instance->_recipes) {
    if (knownRecipes.find(recipe.id()) ==
        knownRecipes.end())  // User doesn't know it yet
      continue;
    if (!_instance->recipeMatchesFilters(recipe))  // Excluded by filters
      continue;
    const ClientItem &product = *toClientItem(recipe.product());
    Element *const recipeElement = new Element({});
    recipesList.addChild(recipeElement);
    recipeElement->addChild(
        new Picture({1, 1, ICON_SIZE, ICON_SIZE}, product.icon()));
    static const px_t NAME_X = ICON_SIZE + CheckBox::GAP + 1;
    recipeElement->addChild(new Label(
        {NAME_X, 0, recipeElement->rect().w - NAME_X, ICON_SIZE + 2},
        recipe.name(), Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
    recipeElement->setLeftMouseUpFunction(selectRecipe);
    recipeElement->id(recipe.id());
  }
  const std::string oldSelectedRecipe = recipesList.getSelected();
  recipesList.verifyBoxes();
  if (recipesList.getSelected() != oldSelectedRecipe)
    selectRecipe(recipesList, {});
}

bool Client::recipeMatchesFilters(const Recipe &recipe) const {
  // "Have materials" filters
  if (_haveMatsFilter) {
    for (const std::pair<const Item *, size_t> &materialsNeeded :
         recipe.materials())
      if (!playerHasItem(materialsNeeded.first, materialsNeeded.second))
        return false;
  }

  // Material filters
  bool matsFilterMatched = !_matFilterSelected || !_matOr;
  if (_matFilterSelected) {
    /*
    "Or": check that the item matches any active material filter.
    Faster to iterate through item's materials, rather than all filters.
    */
    if (_matOr) {
      for (const std::pair<const Item *, size_t> &materialsNeeded :
           recipe.materials()) {
        const ClientItem *const matP = toClientItem(materialsNeeded.first);
        if (_matFilters.find(matP)->second) {
          matsFilterMatched = true;
          break;
        }
      }
      // "And": check that all active filters apply to the item.
    } else {
      for (const std::pair<const ClientItem *, bool> &matFilter : _matFilters) {
        if (!matFilter.second)  // Filter is not active
          continue;
        const ClientItem *const matP = matFilter.first;
        if (!recipe.materials().contains(matP)) return false;
      }
    }
  }

  // Tag filters
  bool tagFilterMatched = !_tagFilterSelected || !_tagOr;
  if (_tagFilterSelected) {
    /*
    "Or": check that the item matches any active tag filter.
    Faster to iterate through item's tags, rather than all filters.
    */
    auto tags = recipe.product()->tags();
    if (_tagOr) {
      for (const std::string &tagName : tags) {
        if (_tagFilters.find(tagName)->second) {
          tagFilterMatched = true;
          break;
        }
      }
      // "And": check that all active filters apply to the item.
    } else {
      for (const std::pair<std::string, bool> &tagFilter : _tagFilters) {
        if (!tagFilter.second)  // Filter is not active
          continue;
        if (tags.find(tagFilter.first) == tags.end()) return false;
      }
    }
  }

  return matsFilterMatched && tagFilterMatched;
}
