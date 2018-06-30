#include "ItemSelector.h"
#include "../Client.h"
#include "Label.h"
#include "Line.h"
#include "Picture.h"
#include "TextBox.h"
#include "Window.h"

const px_t ItemSelector::GAP = 2;
const px_t ItemSelector::LABEL_WIDTH = 120;
const px_t ItemSelector::WIDTH = 80;
const px_t ItemSelector::LABEL_TOP = (ITEM_HEIGHT - TEXT_HEIGHT) / 2;
const px_t ItemSelector::LIST_WIDTH = LABEL_WIDTH + ITEM_HEIGHT + 2 + GAP;
const px_t ItemSelector::LIST_GAP = 0;
const px_t ItemSelector::WINDOW_WIDTH = LIST_WIDTH + 2 * GAP;
const px_t ItemSelector::WINDOW_HEIGHT = 200;
const px_t ItemSelector::SEARCH_BUTTON_WIDTH = 40;
const px_t ItemSelector::SEARCH_BUTTON_HEIGHT = 13;
const px_t ItemSelector::SEARCH_TEXT_WIDTH =
    LIST_WIDTH - GAP - SEARCH_BUTTON_WIDTH;
const px_t ItemSelector::LIST_HEIGHT =
    WINDOW_HEIGHT - SEARCH_BUTTON_HEIGHT - 4 * GAP - 2;

ClientItem **ItemSelector::_itemBeingSelected = nullptr;
Window *ItemSelector::_findItemWindow = nullptr;
TextBox *ItemSelector::_filterText = nullptr;
List *ItemSelector::_itemList = nullptr;

ItemSelector::ItemSelector(const ClientItem *&item, px_t x, px_t y)
    : Button({x, y, WIDTH, Element::ITEM_HEIGHT + 2}, "", openFindItemWindow,
             nullptr),
      _item(item),
      _lastItem(item),
      _icon(new Picture({1, 1, ITEM_HEIGHT, ITEM_HEIGHT}, Texture())),
      _name(new Label({ITEM_HEIGHT + 1 + GAP, 1, WIDTH, ITEM_HEIGHT}, "",
                      Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED)) {
  addChild(_icon);
  addChild(_name);

  clickData(&_item);

  if (_findItemWindow == nullptr) {
    _findItemWindow = Window::WithRectAndTitle(
        {(Client::SCREEN_X - WINDOW_WIDTH) / 2,
         (Client::SCREEN_Y - WINDOW_HEIGHT) / 2, WINDOW_WIDTH, WINDOW_HEIGHT},
        "Find Item");
    px_t y = GAP;
    _filterText = new TextBox({GAP, y, SEARCH_TEXT_WIDTH, SEARCH_BUTTON_HEIGHT},
                              TextBox::LETTERS);
    _findItemWindow->addChild(_filterText);
    _findItemWindow->addChild(
        new Button({SEARCH_TEXT_WIDTH + 2 * GAP, y, SEARCH_BUTTON_WIDTH,
                    SEARCH_BUTTON_HEIGHT},
                   "Search", applyFilter, nullptr));
    y += SEARCH_BUTTON_HEIGHT + GAP;
    _findItemWindow->addChild(new Line(0, y, WINDOW_WIDTH));
    y += 2 + GAP;
    _itemList =
        new List({GAP, y, LIST_WIDTH, LIST_HEIGHT}, ITEM_HEIGHT + 2 + LIST_GAP);
    _findItemWindow->addChild(_itemList);

    applyFilter(nullptr);  // Populate list for the first time.
    Client::_instance->addWindow(_findItemWindow);
  }
}

void ItemSelector::openFindItemWindow(void *data) {
  _findItemWindow->show();
  Client::_instance->removeWindow(_findItemWindow);
  Client::_instance->addWindow(_findItemWindow);
  _itemBeingSelected = static_cast<ClientItem **>(data);
}

void ItemSelector::applyFilter(void *data) {
  _itemList->clearChildren();
  const std::string &filterText = _filterText->text();

  const auto &items = Client::_instance->_items;
  for (const auto &pair : Client::_instance->_items) {
    const ClientItem &item = pair.second;
    if (filterText == "" || itemMatchesFilter(item, filterText)) {
      // Add item to list
      Element *container = new Element();
      _itemList->addChild(container);
      Button *itemButton =
          new Button({0, 0, LIST_WIDTH - List::ARROW_W, ITEM_HEIGHT + 2}, "",
                     selectItem, const_cast<ClientItem *>(&item));
      container->addChild(itemButton);
      itemButton->addChild(
          new Picture({1, 1, ITEM_HEIGHT, ITEM_HEIGHT}, item.icon()));
      itemButton->addChild(
          new Label({ITEM_HEIGHT + GAP, LABEL_TOP, LABEL_WIDTH, TEXT_HEIGHT},
                    item.name()));
    }
  }

  // Add 'none' option
  Element *container = new Element();
  _itemList->addChild(container);
  Button *itemButton =
      new Button({0, 0, LIST_WIDTH - List::ARROW_W, ITEM_HEIGHT + 2}, "",
                 selectItem, nullptr);
  container->addChild(itemButton);
  itemButton->addChild(new Label(
      {ITEM_HEIGHT + GAP, LABEL_TOP, LABEL_WIDTH, TEXT_HEIGHT}, "[None]"));
}

void ItemSelector::selectItem(void *data) {
  *_itemBeingSelected = static_cast<ClientItem *>(data);
  _findItemWindow->hide();
}

bool ItemSelector::itemMatchesFilter(const ClientItem &item,
                                     const std::string &filter) {
  // Name matches
  if (item.name().find(filter) != std::string::npos) return true;

  // Tag matches
  for (const std::string &tagName : item.tags())
    if (tagName.find(filter) != std::string::npos) return true;

  return false;
}

void ItemSelector::checkIfChanged() {
  if (_lastItem != _item) {
    _lastItem = _item;
    markChanged();
  }
  Element::checkIfChanged();
}

void ItemSelector::refresh() {
  if (_item == nullptr) {
    _icon->changeTexture();
    _name->changeText("[None]");
  } else {
    _icon->changeTexture(_item->icon());
    _name->changeText(_item->name());
  }
}
