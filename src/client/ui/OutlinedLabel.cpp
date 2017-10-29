#include "Label.h"
#include "OutlinedLabel.h"

OutlinedLabel::OutlinedLabel(const Rect &rect, const std::string & text,
    Element::Justification justificationH, Element::Justification justificationV):
Element(rect){
    const auto
        w = rect.w - 2,
        h = rect.h - 2;
    auto
        u = new Label({ 1, 0, w, h }, text, justificationH, justificationV),
        d = new Label({ 1, 2, w, h }, text, justificationH, justificationV),
        l = new Label({ 0, 1, w, h }, text, justificationH, justificationV),
        r = new Label({ 2, 1, w, h }, text, justificationH, justificationV),
        _central = new Label({ 1, 1, w, h }, text, justificationH, justificationV);

    u->setColor(Color::OUTLINE);
    d->setColor(Color::OUTLINE);
    l->setColor(Color::OUTLINE);
    r->setColor(Color::OUTLINE);

    addChild(u);
    addChild(d);
    addChild(l);
    addChild(r);
    addChild(_central);
}
