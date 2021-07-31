#include "OutlinedLabel.h"
#include "Label.h"

OutlinedLabel::OutlinedLabel(const ScreenRect &rect, const std::string &text,
                             Element::Justification justificationH,
                             Element::Justification justificationV)
    : Element(rect) {
  const auto w = rect.w - 2, h = rect.h - 2;

  for (auto x = 0; x <= 2; ++x)
    for (auto y = 0; y <= 2; ++y) {
      if (x == 1 && y == 1) continue;  // centre will be handled later

      const auto outline =
          new Label({x, y, w, h}, text, justificationH, justificationV);
      outline->setColor(Color::UI_OUTLINE);
      _labels.push_back(outline);
    }

  _mainText = new Label({1, 1, w, h}, text, justificationH, justificationV);
  _labels.push_back(_mainText);  // Must be last, so that it's drawn in front

  for (auto *label : _labels) addChild(label);
}

void OutlinedLabel::setColor(const Color &color) { _mainText->setColor(color); }

void OutlinedLabel::changeText(const std::string &text) {
  for (auto *label : _labels) label->changeText(text);
}

void OutlinedLabel::refresh() {
  auto width = rect().w;
  auto height = rect().h;

  for (auto *label : _labels) {
    label->width(width);
    label->height(height);
  }
}
