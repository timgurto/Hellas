// (C) 2016 Tim Gurto

#include "ProgressBar.h"
#include "ShadowBox.h"

extern Renderer renderer;

ProgressBar::ProgressBar(const Rect &rect, const unsigned &numerator, const unsigned &denominator):
Element(rect),
_numerator(numerator),
_denominator(denominator),
_lastNumeratorVal(numerator),
_lastDenominatorVal(denominator)
{
    addChild(new ShadowBox(rect, true));
    _bar = new ColorBlock(rect + Rect(1, 1, -2, -2), Element::FONT_COLOR);
    addChild(_bar);
}

void ProgressBar::checkIfChanged(){
    if (_lastNumeratorVal != _numerator) {
        _lastNumeratorVal = _numerator;
        markChanged();
    }
    if (_lastDenominatorVal != _denominator) {
        _lastDenominatorVal = _denominator;
        markChanged();
    }
    Element::checkIfChanged();
}

void ProgressBar::refresh(){
    if (_denominator == 0 || _numerator >= _denominator)
        _bar->width(width() - 2);
    else
        _bar->width(toInt(1.0 * _numerator / _denominator * (width() - 2)));
}
