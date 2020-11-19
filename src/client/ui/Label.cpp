#include "Label.h"

extern Renderer renderer;

Label::Label(const ScreenRect &rect, const std::string &text,
             Justification justificationH, Justification justificationV)
    : Element(rect),
      _text(text),
      _justificationH(justificationH),
      _justificationV(justificationV),
      _matchWidth(false),
      _color(FONT_COLOR) {}

void Label::refresh() {
  Texture text(font(), _text, _color);
  if (_matchWidth) width(text.width());

  px_t x;
  switch (_justificationH) {
    case RIGHT_JUSTIFIED:
      x = rect().w - text.width();
      break;
    case CENTER_JUSTIFIED:
      x = (rect().w - text.width()) / 2;
      break;
    case LEFT_JUSTIFIED:
    default:
      x = 0;
  }

  px_t y;
  switch (_justificationV) {
    case BOTTOM_JUSTIFIED:
      y = rect().h - text.height();
      break;
    case CENTER_JUSTIFIED:
      y = (rect().h - text.height()) / 2;
      break;
    case TOP_JUSTIFIED:
    default:
      y = 0;
  }

  text.draw(x, y + textOffset);
}

void Label::matchW() { _matchWidth = true; }

void Label::setColor(const Color &color) {
  _color = color;
  markChanged();
}

void Label::setJustificationH(const Justification justification) {
  _justificationH = justification;
}

void Label::changeText(const std::string &text) {
  _text = text;
  markChanged();
}
