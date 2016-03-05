// (C) 2016 Tim Gurto

#include "ShadowBox.h"
#include "TextBox.h"
#include "../Renderer.h"

extern Renderer renderer;

const TextBox *TextBox::currentFocus = 0;
const int TextBox::HEIGHT = 14;

TextBox::TextBox(const Rect &rect):
Element(Rect(rect.x, rect.y, rect.w, HEIGHT)),
_text("asdf")
{
    addChild(new ShadowBox(Rect(0, 0, rect.w, HEIGHT), true));
    setLeftMouseDownFunction(&click);
}

void TextBox::text(const std::string &text){
    _text = text;
    markChanged();
}

void TextBox::refresh(){
    // Background
    renderer.setDrawColor(Element::BACKGROUND_COLOR);
    renderer.fill();

    // Text
    static const int
        TEXT_GAP = 2;
    Texture text(Element::font(), _text);
    text.draw(TEXT_GAP, TEXT_GAP);

    // Cursor
    const static int
        CURSOR_GAP = 0,
        CURSOR_WIDTH = 1;
    if (currentFocus == this) {
        renderer.setDrawColor(Element::FONT_COLOR);
        renderer.fillRect(Rect(TEXT_GAP + text.width() + CURSOR_GAP, 1, CURSOR_WIDTH, HEIGHT - 2));
    }
}

void TextBox::click(Element &e, const Point &mousePos){
    //if (!collision(mousePos, e.dimRect()))
    //    return;

    const TextBox *newFocus = dynamic_cast<const TextBox *>(&e);
    if (newFocus == currentFocus)
        return;

    // Mark changed, to (un)draw cursor
    e.markChanged();
    if (currentFocus)
        currentFocus->markChanged();

    currentFocus = newFocus;
}
