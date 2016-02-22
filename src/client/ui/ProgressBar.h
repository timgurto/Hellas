// (C) 2016 Tim Gurto

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include "ColorBlock.h"
#include "Element.h"

// A partially-filled bar indicating a fractional relationship between two numbers.
class ProgressBar : public Element{
public:

private:
    ColorBlock *_bar;

    const unsigned
        &_numerator,
        &_denominator;
    unsigned
        _lastNumeratorVal,
        _lastDenominatorVal;

    virtual void checkIfChanged() override;
    
    virtual void refresh() override;

public:
    ProgressBar(const Rect &rect, const unsigned &numerator, const unsigned &denominator);
};

#endif
