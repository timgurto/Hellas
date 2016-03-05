// (C) 2016 Tim Gurto

#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>

#include "Element.h"
#include "LinkedLabel.h"

class TextBox : public Element{
    std::string _text;

    static const size_t MAX_TEXT_LENGTH;

    static const int
        HEIGHT;

    static TextBox *currentFocus;

public:
    TextBox(const Rect &rect);

    const std::string &text() const { return _text; }
    void text(const std::string &text);

    static void clearFocus();
    static const TextBox *focus() { return currentFocus; }

    static void addText(const char *newText);
    static void backspace();

    virtual void refresh();

    static void click(Element &e, const Point &mousePos);
};

#endif