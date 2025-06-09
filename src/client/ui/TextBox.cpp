#include "TextBox.h"

#include <cassert>

#include "../Renderer.h"
#include "../SDLwrappers.h"
#include "ShadowBox.h"

extern Renderer renderer;

TextBox::TextBox(TextEntryManager &manager, const ScreenRect &rect,
                 ValidInput validInput)
    : Element({rect.x, rect.y, rect.w, max(HEIGHT, rect.h)}),
      _manager(manager),
      _validInput(validInput) {
  addChild(new ShadowBox({0, 0, rect.w, max(HEIGHT, rect.h)}, true));
  setLeftMouseDownFunction(&click);
}

void TextBox::text(const std::string &text) {
  _text = text;
  markChanged();
}

void TextBox::refresh() {
  // Background
  renderer.setDrawColor(Element::BACKGROUND_COLOR);
  renderer.fill();

  // Text
  static const px_t TEXT_GAP = 2;
  auto textToDraw = _text;
  if (_inputMask.hasValue())
    textToDraw = std::string(_text.size(), _inputMask.value());
  Texture text(Element::font(), textToDraw);
  auto textX = TEXT_GAP;
  if (text.width() > width() - 2 * TEXT_GAP)
    textX = width() - text.width() - 2 * TEXT_GAP;
  text.draw(textX, TEXT_GAP);

  // Cursor
  const static px_t CURSOR_GAP = 0, CURSOR_WIDTH = 1;
  if (_manager.textBoxInFocus == this) {
    renderer.setDrawColor(Element::FONT_COLOR);
    renderer.fillRect(
        {textX + text.width() + CURSOR_GAP, 1, CURSOR_WIDTH, height() - 2});
  }
}

void TextBox::click(Element &e, const ScreenPoint &) {
  TextBox *newFocus = dynamic_cast<TextBox *>(&e);
  auto &currentFocus = newFocus->_manager.textBoxInFocus;
  if (newFocus == currentFocus) return;

  currentFocus = newFocus;
}

void TextBox::forcePascalCase() { _text = toPascal(_text); }

void TextBox::setOnChange(OnChangeFunction function, void *data) {
  _onChangeFunction = function;
  _onChangeData = data;
}

void TextBox::addText(TextEntryManager &manager, const char *newText) {
  if (!manager.textBoxInFocus) return;
  assert(newText[1] == '\0');

  const auto &newChar = newText[0];
  if (!manager.textBoxInFocus->isInputValid(newChar)) return;

  std::string &text = manager.textBoxInFocus->_text;
  if (text.size() < MAX_TEXT_LENGTH) {
    text.append(newText);
    manager.textBoxInFocus->onChange();
    manager.textBoxInFocus->markChanged();
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
    default:;  // All allowed
  }
  return true;
}

void TextBox::onChange() {
  if (_onChangeFunction) _onChangeFunction(_onChangeData);
}

void TextBox::backspace(TextEntryManager &manager) {
  assert(manager.textBoxInFocus);

  std::string &text = manager.textBoxInFocus->_text;
  if (text.size() > 0) {
    text.erase(text.size() - 1);
    manager.textBoxInFocus->onChange();
    manager.textBoxInFocus->markChanged();
  }
}

size_t TextBox::textAsNum() const {
  std::istringstream iss(_text);
  int n;
  iss >> n;
  return n;
}

TextBox::Focus &TextBox::Focus::operator=(TextBox *rhs) {
  auto oldFocus = _focus;
  _focus = rhs;

  if (oldFocus) oldFocus->markChanged();
  if (_focus) _focus->markChanged();

  if (_focus && !SDL_IsTextInputActive())
    SDLWrappers::StartTextInput();
  else if (!_focus && SDL_IsTextInputActive())
    SDLWrappers::StopTextInput();

  return *this;
}
