// (C) 2015 Tim Gurto

#include <cassert>

#include "Client.h"
#include "Renderer.h"
#include "ui/Button.h"
#include "ui/CheckBox.h"
#include "ui/Element.h"
#include "ui/Label.h"
#include "ui/Line.h"

extern Renderer renderer;

void Client::initializeCraftingWindow(){
    // For crafting filters
    for (const Recipe &recipe : _recipes) {
        for (auto matPair : recipe.materials())
            _matFilters[matPair.first] = false;
        for (const std::string &className : recipe.product()->classes())
            _classFilters[className] = false;
    }
    _haveMatsFilter = true;
    _classOr = _matOr = false;
    _haveToolsFilter = true;
    _classFilterSelected = _matFilterSelected = false;


    // Set up crafting window
    static const int
        FILTERS_PANE_W = 150,
        RECIPES_PANE_W = 160,
        DETAILS_PANE_W = 150,
        PANE_GAP = 6,
        FILTERS_PANE_X = PANE_GAP / 2,
        RECIPES_PANE_X = FILTERS_PANE_X + FILTERS_PANE_W + PANE_GAP,
        DETAILS_PANE_X = RECIPES_PANE_X + RECIPES_PANE_W + PANE_GAP,
        CRAFTING_WINDOW_W = DETAILS_PANE_X + DETAILS_PANE_W + PANE_GAP/2,

        CONTENT_H = 200,
        CONTENT_Y = PANE_GAP/2,
        CRAFTING_WINDOW_H = CONTENT_Y + CONTENT_H + PANE_GAP/2;

    _craftingWindow = new Window(Rect(100, 50, CRAFTING_WINDOW_W, CRAFTING_WINDOW_H), "Crafting");
    _craftingWindow->addChild(new Line(RECIPES_PANE_X - PANE_GAP/2, CONTENT_Y,
                                       CONTENT_H, Element::VERTICAL));
    _craftingWindow->addChild(new Line(DETAILS_PANE_X - PANE_GAP/2, CONTENT_Y,
                                       CONTENT_H, Element::VERTICAL));

    // Filters
    Element *const filterPane = new Element(Rect(FILTERS_PANE_X, CONTENT_Y,
                                                 FILTERS_PANE_W, CONTENT_H));
    _craftingWindow->addChild(filterPane);
    filterPane->addChild(new Label(Rect(0, 0, FILTERS_PANE_W, HEADING_HEIGHT),
                                   "Filters", Element::CENTER_JUSTIFIED));
    int y = HEADING_HEIGHT;
    CheckBox *pCB = new CheckBox(Rect(0, y, FILTERS_PANE_W / 2, Element::TEXT_HEIGHT),
                                 _haveMatsFilter, "Have materials:");
    pCB->setTooltip("Only show recipes for which you have the materials");
    filterPane->addChild(pCB);
    y += Element::TEXT_HEIGHT;
    filterPane->addChild(new Line(0, y + LINE_GAP/2, FILTERS_PANE_W));
    y += LINE_GAP;

    static const int
        TOTAL_FILTERS_HEIGHT = CONTENT_H - y - 4 * Element::TEXT_HEIGHT - LINE_GAP,
        //TOTAL_FILTERS_HEIGHT = CRAFTING_WINDOW_H - PANE_GAP/2 - y - 4 * Element::TEXT_HEIGHT - LINE_GAP,
        CLASS_LIST_HEIGHT = TOTAL_FILTERS_HEIGHT / 2,
        MATERIALS_LIST_HEIGHT = TOTAL_FILTERS_HEIGHT - CLASS_LIST_HEIGHT;

    // Class filters
    filterPane->addChild(new Label(Rect(0, y, FILTERS_PANE_W, Element::TEXT_HEIGHT),
                                   "Item class:"));
    y += Element::TEXT_HEIGHT;
    List *const classList = new List(Rect(0, y, FILTERS_PANE_W, CLASS_LIST_HEIGHT),
                                     Element::TEXT_HEIGHT);
    filterPane->addChild(classList);
    for (std::map<std::string, bool>::iterator it = _classFilters.begin();
         it != _classFilters.end(); ++it)
        classList->addChild(new CheckBox(Rect(0, 0, FILTERS_PANE_W, Element::TEXT_HEIGHT),
                                         it->second, it->first));
    y += CLASS_LIST_HEIGHT;

    pCB = new CheckBox(Rect(0, y, FILTERS_PANE_W/4, Element::TEXT_HEIGHT), _classOr, "Any");
    pCB->setTooltip("Only show recipes whose product is at least one of the selected classes.");
    filterPane->addChild(pCB);

    pCB = new CheckBox(Rect(FILTERS_PANE_W/2, y, FILTERS_PANE_W/4, Element::TEXT_HEIGHT), _classOr,
                       "All", true);
    pCB->setTooltip("Only show recipes whose product is all of the selected classes.");
    filterPane->addChild(pCB);

    y += Element::TEXT_HEIGHT;
    filterPane->addChild(new Line(0, y + LINE_GAP/2, FILTERS_PANE_W));
    y += LINE_GAP;

    // Material filters
    filterPane->addChild(new Label(Rect(0, y, FILTERS_PANE_W, Element::TEXT_HEIGHT), "Materials:"));
    y += Element::TEXT_HEIGHT;
    List *const materialsList = new List(Rect(0, y, FILTERS_PANE_W, MATERIALS_LIST_HEIGHT),
                                         ICON_SIZE);
    filterPane->addChild(materialsList);
    for (auto it = _matFilters.begin(); it != _matFilters.end(); ++it){
        CheckBox *const mat = new CheckBox(Rect(0, 0, FILTERS_PANE_W, ICON_SIZE), it->second);
        static const int
            ICON_X = CheckBox::BOX_SIZE + CheckBox::GAP,
            LABEL_X = ICON_X + ICON_SIZE + CheckBox::GAP,
            LABEL_W = FILTERS_PANE_W - LABEL_X;
        mat->addChild(new Picture(Rect(ICON_X, 0, ICON_SIZE, ICON_SIZE), it->first->icon()));
        mat->addChild(new Label(Rect(LABEL_X, 0, LABEL_W, ICON_SIZE), it->first->name(),
                                Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
        materialsList->addChild(mat);
    }
    y += MATERIALS_LIST_HEIGHT;
    pCB = new CheckBox(Rect(0, y, FILTERS_PANE_W/4, Element::TEXT_HEIGHT), _matOr, "Any");
    pCB->setTooltip("Only show recipes that require at least one of the selected materials.");
    filterPane->addChild(pCB);

    pCB = new CheckBox(Rect(FILTERS_PANE_W/2, y, FILTERS_PANE_W/4, Element::TEXT_HEIGHT), _matOr,
                                 "All", true);
    pCB->setTooltip("Only show recipes that require all of the selected materials.");
    filterPane->addChild(pCB);

    // Recipes
    Element *const recipesPane = new Element(Rect(RECIPES_PANE_X, CONTENT_Y,
                                                  RECIPES_PANE_W, CONTENT_H));
    _craftingWindow->addChild(recipesPane);
    
    recipesPane->addChild(new Label(Rect(0, 0, RECIPES_PANE_W, HEADING_HEIGHT), "Recipes",
                                    Element::CENTER_JUSTIFIED));
    _recipeList = new ChoiceList(Rect(0, HEADING_HEIGHT,
                                      RECIPES_PANE_W, CONTENT_H - HEADING_HEIGHT),
                                 ICON_SIZE + 2);
    recipesPane->addChild(_recipeList);
    // Click on a filter: force recipe list to refresh
    filterPane->setLeftMouseUpFunction([](Element &e, const Point &mousePos){
                                       if (collision(mousePos,
                                                     Rect(0, 0, e.rect().w, e.rect().h)))
                                           e.markChanged();
                                   },
                                   _recipeList);
    // Repopulate recipe list before every refresh
    _recipeList->setPreRefreshFunction(populateRecipesList, _recipeList);

    // Selected Recipe Details
    _detailsPane = new Element(Rect(DETAILS_PANE_X, CONTENT_Y, DETAILS_PANE_W, CONTENT_H));
    _craftingWindow->addChild(_detailsPane);
    selectRecipe(*_detailsPane, Point()); // Fill details pane initially

    renderer.setScale(static_cast<float>(renderer.width()) / SCREEN_X,
                      static_cast<float>(renderer.height()) / SCREEN_Y);
}

void Client::selectRecipe(Element &e, const Point &mousePos){
    if (!collision(mousePos, Rect(0, 0, e.rect().w, e.rect().h)))
        return;
    Element &pane = *_instance->_detailsPane;
    pane.clearChildren();
    const Rect &paneRect = pane.rect();

    // Close Button
    static const int
        BUTTON_HEIGHT = Element::TEXT_HEIGHT + 4,
        BUTTON_WIDTH = 40,
        BUTTON_GAP = 3,
        BUTTON_Y = paneRect.h - BUTTON_HEIGHT;
    pane.addChild(new Button(Rect(paneRect.w - BUTTON_WIDTH, BUTTON_Y,
                                      BUTTON_WIDTH, BUTTON_HEIGHT),
                             "Close", Window::hideWindow, _instance->_craftingWindow));

    // If no recipe selected
    const std::string &selectedID = _instance->_recipeList->getSelected();
    if (selectedID == "") {
        _instance->_activeRecipe = 0;
        return;
    }

    // Crafting Button
    pane.addChild(new Button(Rect(paneRect.w - 2*BUTTON_WIDTH - BUTTON_GAP, BUTTON_Y,
                                      BUTTON_WIDTH, BUTTON_HEIGHT),
                             "Craft", startCrafting, 0));

    const std::set<Recipe>::const_iterator it = _instance->_recipes.find(selectedID);
    if (it == _instance->_recipes.end()) {
        return;
    }
    const Recipe &recipe = *it;
    _instance->_activeRecipe = &recipe;
    const Item &product = *recipe.product();

    // Title
    pane.addChild(new Label(Rect(0, 0, paneRect.w, HEADING_HEIGHT), recipe.product()->name(),
                            Element::CENTER_JUSTIFIED));
    int y = HEADING_HEIGHT;

    // Icon
    pane.addChild(new Picture(Rect(0, y, ICON_SIZE, ICON_SIZE), product.icon()));

    // Class list
    int x = ICON_SIZE + CheckBox::GAP;
    size_t classesRemaining = product.classes().size();
    const int minLineY = y + ICON_SIZE;
    for (const std::string &className : product.classes()) {
        std::string text = className;
        if (--classesRemaining > 0)
            text += ", ";
        Label *const classLabel = new Label(Rect(0, 0, 0, Element::TEXT_HEIGHT), text);
        classLabel->matchW();
        classLabel->refresh();
        const int width = classLabel->rect().w;
        static const int SPACE_WIDTH = 4;
        if (x + width - SPACE_WIDTH > paneRect.w) {
            x = ICON_SIZE + CheckBox::GAP;
            y += Element::TEXT_HEIGHT;
        }
        classLabel->rect(x, y);
        x += width;
        pane.addChild(classLabel);
    }
    y += Element::TEXT_HEIGHT;
    if (y < minLineY)
        y = minLineY;

    // Divider
    pane.addChild(new Line(0, y + LINE_GAP/2, paneRect.w));
    y += LINE_GAP;

    // Materials list
    const int
        TOTAL_LIST_SPACE = BUTTON_Y - BUTTON_GAP - y - 2 * Element::TEXT_HEIGHT,
        MATS_LIST_HEIGHT = TOTAL_LIST_SPACE / 2,
        TOOLS_LIST_HEIGHT = TOTAL_LIST_SPACE - MATS_LIST_HEIGHT;
    pane.addChild(new Label(Rect(0, y, paneRect.w, Element::TEXT_HEIGHT), "Materials:"));
    y += Element::TEXT_HEIGHT;
    List *const matsList = new List(Rect(0, y, paneRect.w, MATS_LIST_HEIGHT), ICON_SIZE);
    y += MATS_LIST_HEIGHT;
    matsList->setTooltip("The materials required for this recipe.");
    pane.addChild(matsList);
    for (const std::pair<const Item *, size_t> & matCost : recipe.materials()) {
        assert (matCost.first);
        const Item &mat = *matCost.first;
        const size_t qty = matCost.second;
        std::string entryText = mat.name();
        if (qty > 1)
            entryText += " x" + makeArgs(qty);
        Element *const entry = new Element(Rect(0, 0, paneRect.w, ICON_SIZE));
        matsList->addChild(entry);
        entry->addChild(new Picture(Rect(0, 0, ICON_SIZE, ICON_SIZE), mat.icon()));
        entry->addChild(new Label(Rect(ICON_SIZE + CheckBox::GAP, 0, paneRect.w, ICON_SIZE),
                        entryText, Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
    }
    pane.addChild(new Label(Rect(0, y, paneRect.w, Element::TEXT_HEIGHT), "Tools:"));
    y += Element::TEXT_HEIGHT;
    List *const toolsList = new List(Rect(0, y, paneRect.w, TOOLS_LIST_HEIGHT), ICON_SIZE);
    toolsList->setTooltip("The tools required for this recipe.");
    pane.addChild(toolsList);
    for (const std::string &tool : recipe.tools()) {
        static const int TOOL_MARGIN = 5;
        Label *entry = new Label(Rect(TOOL_MARGIN, 0, paneRect.w - TOOL_MARGIN, Element::TEXT_HEIGHT), tool);
        toolsList->addChild(entry);
    }
    pane.markChanged();
}

void Client::populateRecipesList(Element &e){
    // Check which filters are applied
    _instance->_matFilterSelected = false;
    for (const std::pair<const Item *, bool> &filter : _instance->_matFilters) {
        if (filter.second) {
            _instance->_matFilterSelected = true;
            break;
        }
    }
    _instance->_classFilterSelected = false;
    for (const std::pair<std::string, bool> &filter : _instance->_classFilters) {
        if (filter.second) {
            _instance->_classFilterSelected = true;
            break;
        }
    }

    ChoiceList &recipesList = dynamic_cast<ChoiceList &>(e);
    recipesList.clearChildren();

    for (const Recipe &recipe : _instance->_recipes) {
        if (!_instance->recipeMatchesFilters(recipe))
            continue;
        const Item &product = *recipe.product();
        Element *const recipeElement = new Element(Rect());
        recipesList.addChild(recipeElement);
        recipeElement->addChild(new Picture(Rect(1, 1, ICON_SIZE, ICON_SIZE), product.icon()));
        static const int NAME_X = ICON_SIZE + CheckBox::GAP + 1;
        recipeElement->addChild(new Label(Rect(NAME_X, 0, recipeElement->rect().w - NAME_X,
                                               ICON_SIZE + 2),
                                          recipe.product()->name(),
                                          Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
        recipeElement->setLeftMouseUpFunction(selectRecipe);
        recipeElement->id(recipe.id());
    }
    const std::string oldSelectedRecipe = recipesList.getSelected();
    recipesList.verifyBoxes();
    if (recipesList.getSelected() != oldSelectedRecipe)
        selectRecipe(recipesList, Point());
}

bool Client::recipeMatchesFilters(const Recipe &recipe) const{
    // "Have materials" filter
    if (_haveMatsFilter) {
        for (const std::pair<const Item *, size_t> &materialsNeeded : recipe.materials())
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
            for (const std::pair<const Item *, size_t> &materialsNeeded : recipe.materials()) {
                const Item *const matP = materialsNeeded.first;
                if (_matFilters.find(matP)->second) {
                    matsFilterMatched = true;
                    break;
                }
            }
        // "And": check that all active filters apply to the item.
        } else {
            for (const std::pair<const Item *, bool> &matFilter : _matFilters) {
                if (!matFilter.second) // Filter is not active
                    continue;
                const Item *const matP = matFilter.first;
                if (!recipe.materials().contains(matP))
                    return false;
            }
        }
    }

    // Class filters
    bool classFilterMatched = !_classFilterSelected || !_classOr;
    if (_classFilterSelected) {
        /*
        "Or": check that the item matches any active class filter.
        Faster to iterate through item's classes, rather than all filters.
        */
        auto classes = recipe.product()->classes();
        if (_classOr) {
            for (const std::string &className : classes) {
                if (_classFilters.find(className)->second) {
                        classFilterMatched = true;
                        break;
                }
            }
        // "And": check that all active filters apply to the item.
        } else {
            for (const std::pair<std::string, bool> &classFilter : _classFilters) {
                if (!classFilter.second) // Filter is not active
                    continue;
                if (classes.find(classFilter.first) == classes.end())
                    return false;
            }
        }
    }

    return matsFilterMatched && classFilterMatched;
}
