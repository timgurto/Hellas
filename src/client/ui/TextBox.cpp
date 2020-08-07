#include "TextBox.h"

#include <cassert>

#include "../Client.h"
#include "../Renderer.h"
#include "ShadowBox.h"

extern Renderer renderer;

TextBox::TextBox(Client &client, const ScreenRect &rect, ValidInput validInput)
    : Element({rect.x, rect.y, rect.w, max(HEIGHT, rect.h)}),
      _validInput(validInput) {
  setClient(client);
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
  if (_client->textBoxInFocus == this) {
    renderer.setDrawColor(Element::FONT_COLOR);
    renderer.fillRect(
        {textX + text.width() + CURSOR_GAP, 1, CURSOR_WIDTH, height() - 2});
  }
}

void TextBox::click(Element &e, const ScreenPoint &mousePos) {
  TextBox *newFocus = dynamic_cast<TextBox *>(&e);
  auto &currentFocus = newFocus->_client->textBoxInFocus;
  if (newFocus == currentFocus) return;

  // Mark changed, to (un)draw cursor
  e.markChanged();
  if (currentFocus) currentFocus->markChanged();

  currentFocus = newFocus;
}

void TextBox::forcePascalCase() { _text = toPascal(_text); }

void TextBox::setOnChange(OnChangeFunction function, void *data) {
  _onChangeFunction = function;
  _onChangeData = data;
}

void TextBox::addText(Client &client, const char *newText) {
  assert(client.textBoxInFocus);
  assert(newText[1] == '\0');

  const auto &newChar = newText[0];
  if (!client.textBoxInFocus->isInputValid(newChar)) return;

  std::string &text = client.textBoxInFocus->_text;
  if (text.size() < MAX_TEXT_LENGTH) {
    text.append(newText);
    client.textBoxInFocus->onChange();
    client.textBoxInFocus->markChanged();
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

void TextBox::backspace(Client &client) {
  assert(client.textBoxInFocus);

  std::string &text = client.textBoxInFocus->_text;
  if (text.size() > 0) {
    text.erase(text.size() - 1);
    client.textBoxInFocus->onChange();
    client.textBoxInFocus->markChanged();
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

  return *this;
}
