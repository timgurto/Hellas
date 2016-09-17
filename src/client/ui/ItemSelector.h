// (C) 2016 Tim Gurto

#ifndef ITEM_SELECTOR_H
#define ITEM_SELECTOR_H

#include "Button.h"

class Client;
class ClientItem;
class Label;
class List;
class Picture;
class TextBox;
class Window;

// A button used to choose an Item, and display that choice.
class ItemSelector : public Button{
    static const px_t
        GAP,
        LABEL_WIDTH,
        WIDTH,
        LABEL_TOP,
        LIST_WIDTH,
        LIST_GAP,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SEARCH_BUTTON_WIDTH,
        SEARCH_BUTTON_HEIGHT,
        SEARCH_TEXT_WIDTH,
        LIST_HEIGHT;

    const ClientItem *&_item; // Reference to the external Item* that this selector will set.
    const ClientItem *_lastItem; // The last item selected; used to detect changes.

    Picture *_icon;
    Label *_name;

public:
    ItemSelector(const ClientItem *&item, px_t x = 0, px_t y = 0);

    const ClientItem *item() const{ return _item; }
    void item(const ClientItem *item) { _item = item; }
    
    virtual void refresh() override;
    virtual void checkIfChanged() override;

    static ClientItem **_itemBeingSelected;

    static Window *_findItemWindow;
    static TextBox *_filterText;
    static List *_itemList;

    static void openFindItemWindow(void *data); // The find-item window, when a selector is clicked.
    static void applyFilter(void *data);
    static void selectItem(void *data);

    static bool itemMatchesFilter(const ClientItem &item, const std::string &filter);

    friend class Client;
};

#endif
