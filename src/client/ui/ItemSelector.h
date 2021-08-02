#ifndef ITEM_SELECTOR_H
#define ITEM_SELECTOR_H

#include "../ClientItem.h"
#include "Button.h"

class Client;
class Label;
class List;
class Picture;
class TextBox;
class Window;

// A button used to choose an Item, and display that choice.
class ItemSelector : public Button {
 public:
  enum FilterType { MATCHING_SEARCH_TERM, IN_CONTAINER };

  static ItemSelector *ShowItemsMatchingSearchTerm(Client &client,
                                                   const ClientItem *&item,
                                                   ScreenPoint position);
  static ItemSelector *ShowItemsInContainer(
      Client &client, const ClientItem *&item, ScreenPoint position,
      const ClientItem::vect_t &linkedContainer);

  const ClientItem *item() const { return _item; }
  void item(const ClientItem *item) { _item = item; }

  virtual void refresh() override;
  virtual void checkIfChanged() override;

 private:
  ItemSelector(Client &client, const ClientItem *&item, FilterType filterType,
               ScreenPoint position,
               const ClientItem::vect_t *linkedContainer = nullptr);

  static const px_t GAP, LABEL_WIDTH, WIDTH, LABEL_TOP, LIST_WIDTH, LIST_GAP,
      WINDOW_WIDTH, WINDOW_HEIGHT, SEARCH_BUTTON_WIDTH, SEARCH_BUTTON_HEIGHT,
      SEARCH_TEXT_WIDTH, LIST_HEIGHT;

  const ClientItem *
      &_item;  // Reference to the external Item* that this selector will set.
  const ClientItem
      *_lastItem;  // The last item selected; used to detect changes.

  Picture *_icon;
  Label *_name;

  static ClientItem **_itemBeingSelected;

  const ClientItem::vect_t *_linkedContainer{nullptr};

  Window *_findItemWindow{nullptr};
  TextBox *_searchText{nullptr};
  List *_itemList{nullptr};
  FilterType _filterType;

  void addSearchTextBox(px_t &y);

  void openFindItemWindow(
      void *data);  // The find-item window, when a selector is clicked.
  void applyFilter();
  std::vector<ClientItem *> itemsMatchingSearchText() const;
  void addItemToList(ClientItem *item);
  void selectItem(void *data);

  static bool itemMatchesSearchText(const ClientItem &item,
                                    const std::string &filter);

  friend class Client;
};

#endif
