// (C) 2015 Tim Gurto

#ifndef CHECK_BOX_H
#define CHECK_BOX_H

#include <string>

#include "Element.h"

// A button which can be clicked, showing visible feedback and performing a function.
class CheckBox : public Element{
private:
    static const int
        BOX_SIZE,
        GAP, // The gap between box and label, if any.
        Y_OFFSET; // Shifts the box vertically

    bool &_linkedBool; // A boolean whose value is tied to this check box
    // _linkedBool's value when last checked.  Used to determine whether a refresh is necessary.
    bool _lastCheckedValue;
    void toggleBool();

    virtual void checkIfChanged();

    bool _mouseButtonDown;
    bool _depressed;

    void depress();
    void release(bool click); // click: whether, on release, the check box will toggle

    static void mouseDown(Element &e);
    static void mouseUp(Element &e, const Point &mousePos);
    static void mouseMove(Element &e, const Point &mousePos);

    virtual void refresh();

public:
    CheckBox(const SDL_Rect &rect, bool &linkedBool, const std::string &caption = "");
};

#endif
