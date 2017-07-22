#ifndef BUTTON_H
#define BUTTON_H

#include <string>

#include "Element.h"
#include "../../Point.h"

class ColorBlock;
class Label;
class ShadowBox;

// A button which can be clicked, showing visible feedback and performing a function.
class Button : public Element{
public:
    typedef void (*clickFun_t)(void *data);

private:
    Element *_content;
    ColorBlock *_background;
    ShadowBox *_border;
    Label *_caption;

    clickFun_t _clickFun;
    void *_clickData; // Data passed to _clickFun().

    bool _mouseButtonDown;
    bool _depressed;

    void depress();
    void release(bool click); // click: whether, on release, the button's _clickFun will be called.

    static void mouseDown(Element &e, const Point &mousePos);
    static void mouseUp(Element &e, const Point &mousePos);
    static void mouseMove(Element &e, const Point &mousePos);

public:
    Button(const Rect &rect, const std::string &caption = "", clickFun_t clickFunction = nullptr,
           void *clickData = nullptr);
    void clickData(void *data = nullptr) { _clickData = data; }
    virtual void addChild(Element *child) override;
    virtual void clearChildren() override;
    virtual Element *findChild(const std::string id) override;
    
    void width(px_t w) override;
    void height(px_t h) override;
};

#endif
