#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>

#include "Element.h"
#include "LinkedLabel.h"

class TextBox : public Element{
public:
    enum ValidInput {
        ALL,
        NUMERALS,
        LETTERS
    };

    TextBox(const ScreenRect &rect, ValidInput validInput = ALL);

    const std::string &text() const { return _text; }
    void text(const std::string &text);
    size_t textAsNum() const;

    static void clearFocus();
    static const TextBox *focus() { return currentFocus; }
    static void focus(TextBox *textBox) { currentFocus = textBox; }

    static void addText(const char *newText);
    static void backspace();

    virtual void refresh();

    static void click(Element &e, const ScreenPoint &mousePos);

private:
    std::string _text;

    ValidInput _validInput;
    bool isInputValid(char c) const;


    static const size_t MAX_TEXT_LENGTH = 100;

    static const px_t HEIGHT = 14;
    static TextBox *currentFocus;
};

#endif