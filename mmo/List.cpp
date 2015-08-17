// (C) 2015 Tim Gurto

#include "Label.h"
#include "Line.h"
#include "List.h"
#include "Renderer.h"
#include "ShadowBox.h"

extern Renderer renderer;

const int List::ARROW_W = 8;
const int List::ARROW_H = 5;
const int List::CURSOR_HEIGHT = 5;

List::List(const SDL_Rect &rect, int childHeight):
Element(rect),
_childHeight(childHeight),
_content(new Element(makeRect(0, 0, rect.w - ARROW_W, 0))),
_scrollBar(new Element(makeRect(rect.w - ARROW_W, 0, ARROW_W, rect.h))),
_cursor(new Element(makeRect(0, 0, ARROW_W, CURSOR_HEIGHT))),
_scrollY(0){
    Element::addChild(_content);
    Element::addChild(_scrollBar);

    // Scroll bar details
    _scrollBar->addChild(new Line(ARROW_W/2 - 1, ARROW_H, rect.h - 2*ARROW_H, VERTICAL));

    _whiteUp = new Picture(makeRect(0, 0, ARROW_W, ARROW_H),
                           Texture("Images/arrowUp.png", Color::MAGENTA));
    _greyUp = new Picture(makeRect(0, 0, ARROW_W, ARROW_H),
                          Texture("Images/arrowUpGrey.png", Color::MAGENTA));
    _whiteDown = new Picture(makeRect(0, rect.h - ARROW_H, ARROW_W, ARROW_H),
                             Texture("Images/arrowDown.png", Color::MAGENTA)),
    _greyDown = new Picture(makeRect(0, rect.h - ARROW_H, ARROW_W, ARROW_H),
                            Texture("Images/arrowDownGrey.png", Color::MAGENTA));
    _whiteUp->hide();
    _greyDown->hide();
    _scrollBar->addChild(_whiteUp);
    _scrollBar->addChild(_greyUp);
    _scrollBar->addChild(_whiteDown);
    _scrollBar->addChild(_greyDown);

    _cursor->fillBackground();
    _cursor->addChild(new ShadowBox(_cursor->rect()));
    _scrollBar->addChild(_cursor);
    updateCursor();
}

void List::updateCursor(){
    static const int Y_MIN = ARROW_H - 1;
    const int Y_MAX = _scrollBar->rect().h - ARROW_H + 1;
    const int Y_RANGE = Y_MAX - Y_MIN;
    double progress = -1.0 * _content->rect().y / (_content->rect().h - rect().h);
    _cursor->rect(0, static_cast<int>(progress * Y_RANGE + Y_MIN + .5));
}

void List::refresh(){
    // Hide scroll bar if not needed
    if (_content->rect().h <= rect().h)
        _scrollBar->hide();
    else
        _scrollBar->show();

    renderer.pushRenderTarget(_texture);

    drawChildren();

    renderer.popRenderTarget();
}

void List::addChild(Element *child){
    child->rect(0,
                _content->height(),
                rect().w - ARROW_W,
                _childHeight);
    _content->height(_content->height() + _childHeight);
    _content->addChild(child);
}

Element *List::findChild(const std::string id){
    return _content->findChild(id);
}
