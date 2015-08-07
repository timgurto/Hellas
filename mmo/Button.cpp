// (C) 2015 Tim Gurto

#include "Button.h"
#include "Label.h"
#include "ShadowBox.h"

extern Renderer renderer;

Button::Button(const SDL_Rect &rect, const std::string &caption):
Element(rect),
_caption(caption){
    addChild(new ShadowBox(makeRect(0, 0, rect.w, rect.h)));
    if (!caption.empty())
        addChild(new Label(makeRect(0, 0, rect.w, rect.h),
                           caption, CENTER_JUSTIFIED, CENTER_JUSTIFIED));

}

void Button::refresh(){
    renderer.pushRenderTarget(_texture);

    renderer.setDrawColor(BACKGROUND_COLOR);
    renderer.fillRect(makeRect(1, 1, rect().w - 2, rect().h - 2));

    drawChildren();

    renderer.popRenderTarget();
}
