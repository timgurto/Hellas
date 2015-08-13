// (C) 2015 Tim Gurto

#include "Label.h"
#include "List.h"
#include "Renderer.h"

extern Renderer renderer;

List::List(const SDL_Rect &rect, int childHeight):
Element(rect),
_childHeight(childHeight),
_content(new Element(makeRect(0, 0, rect.w - SCROLL_BAR_WIDTH, 0))){
    Element::addChild(_content);
    addChild(new Label(makeRect(), "label"));
}

void List::refresh(){
    renderer.pushRenderTarget(_texture);

    drawChildren();

    renderer.popRenderTarget();
}

void List::addChild(Element *child){
    child->rect(0,
                _content->height(),
                rect().w - SCROLL_BAR_WIDTH,
                _childHeight);
    _content->height(_content->height() + _childHeight);
    _content->addChild(child);
}

Element *List::findChild(const std::string id){
    return _content->findChild(id);
}
