#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include "ColorBlock.h"
#include "Element.h"

// A partially-filled bar indicating a fractional relationship between two numbers.
template<typename T>
class ProgressBar : public Element{
public:

private:
    ColorBlock *_bar;

    const T
        &_numerator,
        &_denominator;
    T
        _lastNumeratorVal,
        _lastDenominatorVal;

    virtual void checkIfChanged() override;

public:
    ProgressBar(const Rect &rect, const T &numerator, const T &denominator,
               const Color&barColor = Color::PROGRESS_BAR,
               const Color &backgroundColor = Color::PROGRESS_BAR_BACKGROUND);
    void changeColor(const Color &newColor) { _bar->changeColor(newColor); }
};


// Implementations

extern Renderer renderer;

template<typename T>
ProgressBar<T>::ProgressBar(const Rect &rect, const T &numerator, const T &denominator,
                            const Color &barColor, const Color &backgroundColor):
Element(rect),
_numerator(numerator),
_denominator(denominator),
_lastNumeratorVal(numerator),
_lastDenominatorVal(denominator)
{
    addChild(new ColorBlock(Rect(1, 1, rect.w - 2, rect.h - 2), backgroundColor));
    addChild(new ShadowBox(Rect(0, 0, rect.w, rect.h), true));
    _bar = new ColorBlock(Rect(1, 1, rect.w - 2, rect.h - 2), barColor);
    addChild(_bar);
}

template<typename T>
void ProgressBar<T>::checkIfChanged(){
    bool changed = false;
    if (_lastNumeratorVal != _numerator) {
        _lastNumeratorVal = _numerator;
        changed = true;
    }
    if (_lastDenominatorVal != _denominator) {
        _lastDenominatorVal = _denominator;
        changed = true;
    }
    if (true) {
        if (_denominator == 0 || _numerator >= _denominator)
            _bar->width(width() - 2);
        else
            _bar->width(toInt(1.0 * _numerator / _denominator * (width() - 2)));
        markChanged();
    }
    Element::checkIfChanged();
}

#endif
