#include <cassert>

#include "ShadowBox.h"
#include "TextBox.h"
#include "../Renderer.h"

extern Renderer renderer;

TextBox *TextBox::currentFocus = nullptr;
const px_t TextBox::HEIGHT = 14;
const size_t TextBox::MAX_TEXT_LENGTH = 100;

TextBox::TextBox(const ScreenRect &rect, ValidInput validInput):
Element({rect.x, rect.y, rect.w, HEIGHT}),
_validInput(validInput)
{
    addChild(new ShadowBox({ 0, 0, rect.w, HEIGHT }, true));
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
    static const px_t
        TEXT_GAP = 2;
    Texture text(Element::font(), _text);
    text.draw(TEXT_GAP, TEXT_GAP);

    // Cursor
    const static px_t
        CURSOR_GAP = 0,
        CURSOR_WIDTH = 1;
    if (currentFocus == this) {
        renderer.setDrawColor(Element::FONT_COLOR);
        renderer.fillRect({ TEXT_GAP + text.width() + CURSOR_GAP, 1, CURSOR_WIDTH, HEIGHT - 2 });
    }
}

void TextBox::clearFocus(){
    if (currentFocus != nullptr)
        currentFocus->markChanged();
    currentFocus = nullptr;
}

void TextBox::click(Element &e, const ScreenPoint &mousePos){
    TextBox *newFocus = dynamic_cast<TextBox *>(&e);
    if (newFocus == currentFocus)
        return;

    // Mark changed, to (un)draw cursor
    e.markChanged();
    if (currentFocus != nullptr)
        currentFocus->markChanged();

    currentFocus = newFocus;
}

void TextBox::addText(const char *newText){
    assert(currentFocus);
    assert(newText[1] == '\0');

    const auto &newChar = newText[0];
    if (!currentFocus->isInputValid(newChar))
        return;

    std::string &text = currentFocus->_text;
    if (text.size() < MAX_TEXT_LENGTH) {
        text.append(newText);
        currentFocus->markChanged();
    }
}

bool TextBox::isInputValid(char c) const {
    switch (_validInput) {
    case NUMERALS:
        if (c < '0' || c > '9') return false;
        break;
    case LETTERS:
        if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z')) return false;
        break;
    default:
        ; // All allowed
    }
    return true;
}

void TextBox::backspace(){
    assert(currentFocus);

    std::string &text = currentFocus->_text;
    if (text.size() > 0) {
        text.erase(text.size() - 1);
        currentFocus->markChanged();
    }
}

size_t TextBox::textAsNum() const{
    std::istringstream iss(_text);
    int n;
    iss >> n;
    return n;
}
