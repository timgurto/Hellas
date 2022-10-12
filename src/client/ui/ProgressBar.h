#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include "ColorBlock.h"
#include "Element.h"

using namespace std::string_literals;

// A partially-filled bar indicating a fractional relationship between two
// numbers.
template <typename T>
class ProgressBar : public Element {
 public:
 protected:  // TODO make as much private as possible
  ColorBlock *_bar;

  const T &_numerator, &_denominator;
  T _lastNumeratorVal, _lastDenominatorVal;

  bool _showValuesInTooltip = false;
  std::string _tooltipSuffix{};

  virtual std::string tooltipText() const {
    return toString(_numerator) + "/"s + toString(_denominator) +
           _tooltipSuffix;
  }

  void checkIfChanged() override;
  bool hasChanged();
  virtual void updateBarWidth();

  void refresh() override {
    if (_showValuesInTooltip) setTooltip(tooltipText());
    Element::refresh();
  }

 public:
  ProgressBar(const ScreenRect &rect, const T &numerator, const T &denominator,
              const Color &barColor = Color::WINDOW_FONT,
              const Color &backgroundColor = Color::UI_PROGRESS_BAR);
  void changeColor(const Color &newColor) { _bar->changeColor(newColor); }

  void showValuesInTooltip(const std::string &suffix = "") {
    _showValuesInTooltip = true;
    _tooltipSuffix = suffix;
  }
};

// Implementations

template <typename T>
ProgressBar<T>::ProgressBar(const ScreenRect &rect, const T &numerator,
                            const T &denominator, const Color &barColor,
                            const Color &backgroundColor)
    : Element(rect),
      _numerator(numerator),
      _denominator(denominator),
      _lastNumeratorVal(numerator),
      _lastDenominatorVal(denominator) {
  addChild(new ColorBlock({1, 1, rect.w - 2, rect.h - 2}, backgroundColor));
  addChild(new ShadowBox({0, 0, rect.w, rect.h}, true));
  _bar = new ColorBlock({1, 1, rect.w - 2, rect.h - 2}, barColor);
  addChild(_bar);
}

template <typename T>
void ProgressBar<T>::checkIfChanged() {
  if (hasChanged()) {
    if (_denominator == 0 || _numerator >= _denominator)
      _bar->width(width() - 2);
    else
      updateBarWidth();
    markChanged();
  }
  Element::checkIfChanged();
}

template <typename T>
bool ProgressBar<T>::hasChanged() {
  auto changed = false;
  if (_lastNumeratorVal != _numerator) {
    _lastNumeratorVal = _numerator;
    changed = true;
  }
  if (_lastDenominatorVal != _denominator) {
    _lastDenominatorVal = _denominator;
    changed = true;
  }
  return changed;
}

template <typename T>
void ProgressBar<T>::updateBarWidth() {
  _bar->width(toInt(1.0 * _numerator / _denominator * (width() - 2)));
}

template <typename T>
class ProgressBarWithBonus : public ProgressBar<T> {
 private:
  const T &_bonus;
  T _lastBonusVal;
  ColorBlock *_bonusBar;

  void updateBarWidth() override;

  std::string tooltipText() const override {
    auto text = ProgressBar::tooltipText();
    if (_bonus > 0) text += " (+"s + toString(_bonus) + " bonus)"s;
    return text;
  }

 public:
  ProgressBarWithBonus(const ScreenRect &rect, const T &numerator,
                       const T &denominator, const T &bonus,
                       const Color &barColor = Color::WINDOW_FONT,
                       const Color &bonusColor = Color::WINDOW_HEADING,
                       const Color &backgroundColor = Color::UI_PROGRESS_BAR)
      : ProgressBar(rect, numerator, denominator, barColor, backgroundColor),
        _bonus(bonus) {
    _bonusBar = new ColorBlock({1, 1, 1, rect.h - 2}, bonusColor);
    addChild(_bonusBar);
  }
};

template <typename T>
void ProgressBarWithBonus<T>::updateBarWidth() {
  ProgressBar::updateBarWidth();

  const auto spaceRemaining = width() - 2 - _bar->width();
  // Note: doubling _bonus below so that this works with bonus XP
  // (i.e., the bar shows how much XP will have been earned once the bonus
  // XP is all used up.)
  // If this class is used for anything else then this will need to change.
  auto bonusBarWidth = toInt(1.0 * _bonus * 2 / _denominator * (width() - 2));
  bonusBarWidth = min(bonusBarWidth, spaceRemaining);
  _bonusBar->width(bonusBarWidth);
  _bonusBar->left(_bar->right());
}

#endif
