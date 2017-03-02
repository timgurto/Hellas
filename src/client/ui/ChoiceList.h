// (C) 2015-2016 Tim Gurto

#ifndef CHOICE_LIST_H
#define CHOICE_LIST_H

#include <string>

#include "List.h"
#include "ShadowBox.h"

// A list of elements with IDs; up to one item can be chosen at a time.
class ChoiceList : public List{
    // Note that these three IDs may be invalidated by clearChildren().  Check with verifyBoxes().
    std::string _selectedID;
    std::string _mouseOverID;
    std::string _mouseDownID;
    ShadowBox
        *_selectedBox,
        *_mouseOverBox,
        *_mouseDownBox;
    Element *_boxLayer; // shape/position is a copy of List::_content

    const std::string &getIdFromMouse(double mouseY, int *index = nullptr) const;
    bool contentCollision(const Point &p) const;

    static void markMouseDown(Element &e, const Point &mousePos);
    static void toggle(Element &e, const Point &mousePos);
    static void markMouseOver(Element &e, const Point &mousePos);

public:
    ChoiceList(const Rect &rect, px_t childHeight);

    virtual void refresh() override;

    const std::string &getSelected() { return _selectedID; }
    void clearSelection();

    void verifyBoxes(); // Call after changing the list's contents.
};

#endif
