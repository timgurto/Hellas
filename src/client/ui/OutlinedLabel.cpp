#include "Label.h"
#include "OutlinedLabel.h"

OutlinedLabel::OutlinedLabel(const ScreenRect &rect, const std::string & text,
    Element::Justification justificationH, Element::Justification justificationV):
Element(rect){
    const auto
        w = rect.w - 2,
        h = rect.h - 2;
    _u = new Label({ 1, 0, w, h }, text, justificationH, justificationV),
    _d = new Label({ 1, 2, w, h }, text, justificationH, justificationV),
    _l = new Label({ 0, 1, w, h }, text, justificationH, justificationV),
    _r = new Label({ 2, 1, w, h }, text, justificationH, justificationV);
    _central = new Label({ 1, 1, w, h }, text, justificationH, justificationV);

    _u->setColor(Color::OUTLINE);
    _d->setColor(Color::OUTLINE);
    _l->setColor(Color::OUTLINE);
    _r->setColor(Color::OUTLINE);

    addChild(_u);
    addChild(_d);
    addChild(_l);
    addChild(_r);
    addChild(_central);
}

void OutlinedLabel::setColor(const Color &color) {
    _central->setColor(color);
}

void OutlinedLabel::changeText(const std::string &text) {
    _u->changeText(text);
    _d->changeText(text);
    _l->changeText(text);
    _r->changeText(text);
    _central->changeText(text);
}
