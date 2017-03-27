#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>
#include <string>

class Button;
class ColorBlock;
class Element;
class Label;
class Line;
class ShadowBox;

// A generic window for the in-game UI.
class Window : public Element{
    std::string _title;
    bool _dragging; // Whether this window is currently being dragged by the mouse.
    Point _dragOffset; // While dragging, where the mouse is on the window.
    Element *_content;
    ColorBlock *_background;
    ShadowBox *_border;
    Label *_heading;
    Line *_headingLine;
    Button *_closeButton;

public:
    static px_t HEADING_HEIGHT;
    static px_t CLOSE_BUTTON_SIZE;

    Window(const Rect &rect, const std::string &title);

    static void hideWindow(void *window);

    static void startDragging(Element &e, const Point &mousePos);
    static void stopDragging(Element &e, const Point &mousePos);
    static void drag(Element &e, const Point &mousePos);

    void resize(px_t w, px_t h); // Resize window, so that the content size matches the given dims.
    
    px_t contentWidth() { return _content->width(); }
    px_t contentHeight() { return _content->height(); }

    virtual void addChild(Element *child) override;
    virtual void clearChildren() override;
    virtual Element *findChild(const std::string id) override;
};

#endif
