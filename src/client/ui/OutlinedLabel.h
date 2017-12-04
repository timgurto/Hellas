#pragma once

#include "Element.h"

// A label with 
class OutlinedLabel : public Element {
public:
    OutlinedLabel(const ScreenRect &rect, const std::string & text,
        Element::Justification justificationH = Element::LEFT_JUSTIFIED,
        Element::Justification justificationV = Element::TOP_JUSTIFIED);

    Label *centralLabel() { return _central; }

private:
    Label *_central;
};
