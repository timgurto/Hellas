#include "OutlinedLabel.h"
#include "Label.h"

OutlinedLabel::OutlinedLabel(const ScreenRect &rect, const std::string &text,
                             Element::Justification justificationH,
                             Element::Justification justificationV)
    : Element(rect) {
  const auto w = rect.w - 2, h = rect.h - 2;
  _labels.push_back(
      new Label({0, 0, w, h}, text, justificationH, justificationV));
  _labels.push_back(
      new Label({1, 0, w, h}, text, justificationH, justificationV));
  _labels.push_back(
      new Label({2, 0, w, h}, text, justificationH, justificationV));
  _labels.push_back(
      new Label({2, 1, w, h}, text, justificationH, justificationV));
  _labels.push_back(
      new Label({2, 2, w, h}, text, justificationH, justificationV));
  _labels.push_back(
      new Label({1, 2, w, h}, text, justificationH, justificationV));
  _labels.push_back(
      new Label({0, 2, w, h}, text, justificationH, justificationV));
  _labels.push_back(
      new Label({0, 1, w, h}, text, justificationH, justificationV));

  for (auto *outlineLabel : _labels) outlineLabel->setColor(Color::UI_OUTLINE);

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
