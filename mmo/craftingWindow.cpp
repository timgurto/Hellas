// (C) 2015 Tim Gurto

#include <cassert>

#include "Button.h"
#include "CheckBox.h"
#include "Client.h"
#include "Label.h"
#include "Line.h"
#include "Renderer.h"

extern Renderer renderer;

void Client::initializeCraftingWindow(){
    // For crafting filters
    for (const Item &item : _items){
        if (item.isCraftable()) {
            _craftableItems.insert(&item);
            for (const std::pair<const Item *, size_t> mat : item.materials())
                _matFilters[mat.first] = false;
            for (const std::string &className : item.classes())
                 _classFilters[className] = false;
        }
    }
    _haveMatsFilter = _classOr = _matOr = false;
    _haveToolsFilter = true;
    _classFilterSelected = _matFilterSelected = false;


    // Set up crafting window
    static const int
        FILTERS_PANE_W = 100,
        RECIPES_PANE_W = 110,
        DETAILS_PANE_W = 100,
        PANE_GAP = 6,
        FILTERS_PANE_X = PANE_GAP / 2,
        RECIPES_PANE_X = FILTERS_PANE_X + FILTERS_PANE_W + PANE_GAP,
        DETAILS_PANE_X = RECIPES_PANE_X + RECIPES_PANE_W + PANE_GAP,
        CRAFTING_WINDOW_W = DETAILS_PANE_X + DETAILS_PANE_W + PANE_GAP/2,

        CONTENT_H = 200,
        CONTENT_Y = Window::HEADING_HEIGHT + PANE_GAP/2,
        CRAFTING_WINDOW_H = CONTENT_Y + CONTENT_H + PANE_GAP/2;

    _craftingWindow = new Window(makeRect(250, 50, CRAFTING_WINDOW_W, CRAFTING_WINDOW_H),
                                 "Crafting");
    _craftingWindow->addChild(new Line(RECIPES_PANE_X - PANE_GAP/2, CONTENT_Y,
                                       CONTENT_H, Element::VERTICAL));
    _craftingWindow->addChild(new Line(DETAILS_PANE_X - PANE_GAP/2, CONTENT_Y,
                                       CONTENT_H, Element::VERTICAL));

    // Filters
    static const int
        CLASS_LIST_HEIGHT = 59,
        MATERIALS_LIST_HEIGHT = 60;
    Element *const filterPane = new Element(makeRect(FILTERS_PANE_X, CONTENT_Y,
                                                     FILTERS_PANE_W, CONTENT_H));
    _craftingWindow->addChild(filterPane);
    filterPane->addChild(new Label(makeRect(0, 0, FILTERS_PANE_W, HEADING_HEIGHT),
                                   "Filters", Element::CENTER_JUSTIFIED));
    int y = HEADING_HEIGHT;
    filterPane->addChild(new CheckBox(makeRect(0, y, FILTERS_PANE_W, TEXT_HEIGHT),
                                      _haveMatsFilter, "Have materials:"));
    y += TEXT_HEIGHT;
    filterPane->addChild(new Line(0, y + LINE_GAP/2, FILTERS_PANE_W));
    y += LINE_GAP;

    // Class filters
    filterPane->addChild(new Label(makeRect(0, y, FILTERS_PANE_W, TEXT_HEIGHT), "Item class:"));
    y += TEXT_HEIGHT;
    List *const classList = new List(makeRect(0, y, FILTERS_PANE_W, CLASS_LIST_HEIGHT), TEXT_HEIGHT);
    filterPane->addChild(classList);
    for (std::map<std::string, bool>::iterator it = _classFilters.begin();
         it != _classFilters.end(); ++it)
        classList->addChild(new CheckBox(makeRect(0, 0, FILTERS_PANE_W, TEXT_HEIGHT),
                                         it->second, it->first));
    y += CLASS_LIST_HEIGHT;
    filterPane->addChild(new CheckBox(makeRect(0, y, FILTERS_PANE_W/2, TEXT_HEIGHT),
                                      _classOr, "Any"));
    filterPane->addChild(new CheckBox(makeRect(FILTERS_PANE_W/2, y, FILTERS_PANE_W/2, TEXT_HEIGHT),
                                      _classOr, "All", true));
    y += TEXT_HEIGHT;
    filterPane->addChild(new Line(0, y + LINE_GAP/2, FILTERS_PANE_W));
    y += LINE_GAP;

    // Material filters
    filterPane->addChild(new Label(makeRect(0, y, FILTERS_PANE_W, TEXT_HEIGHT), "Materials:"));
    y += TEXT_HEIGHT;
    List *const materialsList = new List(makeRect(0, y, FILTERS_PANE_W, MATERIALS_LIST_HEIGHT),
                                         ICON_SIZE);
    filterPane->addChild(materialsList);
    for (auto it = _matFilters.begin(); it != _matFilters.end(); ++it){
        CheckBox *const mat = new CheckBox(makeRect(0, 0, FILTERS_PANE_W, ICON_SIZE), it->second);
        static const int
            ICON_X = CheckBox::BOX_SIZE + CheckBox::GAP,
            LABEL_X = ICON_X + ICON_SIZE + CheckBox::GAP,
            LABEL_W = FILTERS_PANE_W - LABEL_X;
        mat->addChild(new Picture(makeRect(ICON_X, 0, ICON_SIZE, ICON_SIZE), it->first->icon()));
        mat->addChild(new Label(makeRect(LABEL_X, 0, LABEL_W, ICON_SIZE), it->first->name(),
                                Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
        materialsList->addChild(mat);
    }
    y += MATERIALS_LIST_HEIGHT;
    filterPane->addChild(new CheckBox(makeRect(0, y, FILTERS_PANE_W/2, TEXT_HEIGHT),
                                      _matOr, "Any"));
    filterPane->addChild(new CheckBox(makeRect(FILTERS_PANE_W/2, y, FILTERS_PANE_W/2, TEXT_HEIGHT),
                                      _matOr, "All", true));

    // Recipes
    Element *const recipesPane = new Element(makeRect(RECIPES_PANE_X, CONTENT_Y,
                                                      RECIPES_PANE_W, CONTENT_H));
    _craftingWindow->addChild(recipesPane);
    
    recipesPane->addChild(new Label(makeRect(0, 0, RECIPES_PANE_W, HEADING_HEIGHT), "Recipes",
                                    Element::CENTER_JUSTIFIED));
    _recipeList = new ChoiceList(makeRect(0, HEADING_HEIGHT,
                                                      RECIPES_PANE_W, CONTENT_H - HEADING_HEIGHT),
                                             ICON_SIZE + 2);
    recipesPane->addChild(_recipeList);
    // Click on a filter: force recipe list to refresh
    filterPane->setMouseUpFunction([](Element &e, const Point &mousePos){
                                       if (collision(mousePos,
                                                     makeRect(0, 0, e.rect().w, e.rect().h)))
                                           e.markChanged();
                                   },
                                   _recipeList);
    // Repopulate recipe list before every refresh
    _recipeList->setPreRefreshFunction(populateRecipesList, _recipeList);

    // Selected Recipe Details
    _detailsPane = new Element(makeRect(DETAILS_PANE_X, CONTENT_Y, DETAILS_PANE_W, CONTENT_H));
    _craftingWindow->addChild(_detailsPane);
    selectRecipe(*_detailsPane, Point()); // Fill details pane initially

    renderer.setScale(static_cast<float>(renderer.width()) / SCREEN_X,
                      static_cast<float>(renderer.height()) / SCREEN_Y);
}

void Client::selectRecipe(Element &e, const Point &mousePos){
    if (!collision(mousePos, makeRect(0, 0, e.rect().w, e.rect().h)))
        return;
    Element &pane = *_instance->_detailsPane;
    pane.clearChildren();
    const SDL_Rect &paneRect = pane.rect();

    // Close Button
    static const int
        BUTTON_HEIGHT = 15,
        BUTTON_WIDTH = 40,
        BUTTON_GAP = 3,
        BUTTON_Y = paneRect.h - BUTTON_HEIGHT;
    pane.addChild(new Button(makeRect(paneRect.w - BUTTON_WIDTH, BUTTON_Y,
                                      BUTTON_WIDTH, BUTTON_HEIGHT),
                             "Close", Window::hideWindow, _instance->_craftingWindow));

    // If no recipe selected
    const std::string &selectedID = _instance->_recipeList->getSelected();
    if (selectedID == "") {
        _instance->_activeRecipe = 0;
        return;
    }

    // Crafting Button
    pane.addChild(new Button(makeRect(paneRect.w - 2*BUTTON_WIDTH - BUTTON_GAP, BUTTON_Y,
                                      BUTTON_WIDTH, BUTTON_HEIGHT),
                             "Craft", startCrafting, 0));

    const std::set<Item>::const_iterator it = _instance->_items.find(selectedID);
    if (it == _instance->_items.end()) {
        return;
    }
    const Item &item = *it;
    _instance->_activeRecipe = &item;

    // Title
    pane.addChild(new Label(makeRect(0, 0, paneRect.w, HEADING_HEIGHT), item.name(),
                            Element::CENTER_JUSTIFIED));
    int y = HEADING_HEIGHT;

    // Icon
    pane.addChild(new Picture(makeRect(0, y, ICON_SIZE, ICON_SIZE), item.icon()));

    // Class list
    int x = ICON_SIZE + CheckBox::GAP;
    size_t classesRemaining = item.classes().size();
    const int minLineY = y + ICON_SIZE;
    for (const std::string &className : item.classes()) {
        std::string text = className;
        if (--classesRemaining > 0)
            text += ", ";
        Label *const classLabel = new Label(makeRect(0, 0, 0, TEXT_HEIGHT), text);
        classLabel->matchW();
        classLabel->refresh();
        const int width = classLabel->rect().w;
        static const int SPACE_WIDTH = 4;
        if (x + width - SPACE_WIDTH > paneRect.w) {
            x = ICON_SIZE + CheckBox::GAP;
            y += TEXT_HEIGHT;
        }
        classLabel->rect(x, y);
        x += width;
        pane.addChild(classLabel);
    }
    y += TEXT_HEIGHT;
    if (y < minLineY)
        y = minLineY;

    // Divider
    pane.addChild(new Line(0, y + LINE_GAP/2, paneRect.w));
    y += LINE_GAP;

    // Materials list
    pane.addChild(new Label(makeRect(0, y, paneRect.w, TEXT_HEIGHT), "Materials:"));
    y += TEXT_HEIGHT;
    List *const matsList = new List(makeRect(0, y, paneRect.w, BUTTON_Y - BUTTON_GAP - y),
                                    ICON_SIZE);
    pane.addChild(matsList);
    for (const std::pair<const Item *, size_t> & matCost : item.materials()) {
        assert (matCost.first);
        const Item &mat = *matCost.first;
        const size_t qty = matCost.second;
        std::string entryText = mat.name();
        if (qty > 1)
            entryText += " x" + makeArgs(qty);
        Element *const entry = new Element(makeRect(0, 0, paneRect.w, ICON_SIZE));
        matsList->addChild(entry);
        entry->addChild(new Picture(makeRect(0, 0, ICON_SIZE, ICON_SIZE), mat.icon()));
        entry->addChild(new Label(makeRect(ICON_SIZE + CheckBox::GAP, 0, paneRect.w, ICON_SIZE),
                        entryText, Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
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

    for (const Item *item : _instance->_craftableItems) {
        if (!_instance->itemMatchesFilters(*item))
            continue;
        Element *const recipe = new Element(makeRect());
        recipesList.addChild(recipe);
        recipe->addChild(new Picture(makeRect(1, 1, ICON_SIZE, ICON_SIZE), item->icon()));
        static const int NAME_X = ICON_SIZE + CheckBox::GAP + 1;
        recipe->addChild(new Label(makeRect(NAME_X, 0, recipe->rect().w - NAME_X, ICON_SIZE + 2),
                                   item->name(),
                                   Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
        recipe->setMouseUpFunction(selectRecipe);
        recipe->id(item->id());
    }
    const std::string oldSelectedRecipe = recipesList.getSelected();
    recipesList.verifyBoxes();
    if (recipesList.getSelected() != oldSelectedRecipe)
        selectRecipe(recipesList, Point());
}

bool Client::itemMatchesFilters(const Item &item) const{
    // "Have materials" filter
    if (_haveMatsFilter) {
        for (const std::pair<const Item *, size_t> &materialsNeeded : item.materials())
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
            for (const std::pair<const Item *, size_t> &materialsNeeded : item.materials()) {
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
                if (item.materials().find(matP) == item.materials().end())
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
        if (_classOr) {
            for (const std::string &className : item.classes()) {
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
                if (item.classes().find(classFilter.first) == item.classes().end())
                    return false;
            }
        }
    }

    return matsFilterMatched && classFilterMatched;
}
