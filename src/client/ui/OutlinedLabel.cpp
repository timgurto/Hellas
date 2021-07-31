#include "OutlinedLabel.h"
#include "Label.h"

OutlinedLabel::OutlinedLabel(const ScreenRect &rect, const std::string &text,
                             Element::Justification justificationH,
                             Element::Justification justificationV)
    : Element(rect) {
  const auto w = rect.w - 2, h = rect.h - 2;
  _u = new Label({1, 0, w, h}, text, justificationH, justificationV),
  _d = new Label({1, 2, w, h}, text, justificationH, justificationV),
  _l = new Label({0, 1, w, h}, text, justificationH, justificationV),
  _r = new Label({2, 1, w, h}, text, justificationH, justificationV);
  _e = new Label({2, 0, w, h}, text, justificationH, justificationV);
  _f = new Label({2, 2, w, h}, text, justificationH, justificationV);
  _g = new Label({0, 2, w, h}, text, justificationH, justificationV);
  _h = new Label({0, 0, w, h}, text, justificationH, justificationV);
  _central = new Label({1, 1, w, h}, text, justificationH, justificationV);

  _u->setColor(Color::UI_OUTLINE);
  _d->setColor(Color::UI_OUTLINE);
  _l->setColor(Color::UI_OUTLINE);
  _r->setColor(Color::UI_OUTLINE);
  _e->setColor(Color::UI_OUTLINE);
  _f->setColor(Color::UI_OUTLINE);
  _g->setColor(Color::UI_OUTLINE);
  _h->setColor(Color::UI_OUTLINE);

  addChild(_u);
  addChild(_d);
  addChild(_l);
  addChild(_r);
  addChild(_e);
  addChild(_f);
  addChild(_g);
  addChild(_h);
  addChild(_central);
}

void OutlinedLabel::setColor(const Color &color) { _central->setColor(color); }

void OutlinedLabel::changeText(const std::string &text) {
  _u->changeText(text);
  _d->changeText(text);
  _l->changeText(text);
  _r->changeText(text);
  _e->changeText(text);
  _f->changeText(text);
  _g->changeText(text);
  _h->changeText(text);
  _central->changeText(text);
}

void OutlinedLabel::refresh() {
  auto width = rect().w;
  _u->width(width);
  _d->width(width);
  _l->width(width);
  _r->width(width);
  _e->width(width);
  _f->width(width);
  _g->width(width);
  _h->width(width);
  _central->width(width);

  auto height = rect().h;
  _u->height(height);
  _d->height(height);
  _l->height(height);
  _r->height(height);
  _e->height(height);
  _f->height(height);
  _g->height(height);
  _h->height(height);
  _central->height(height);
}
