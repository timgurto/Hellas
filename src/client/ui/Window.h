#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>
#include <string>

class Client;
class Button;
class ColorBlock;
class Element;
class Label;
class Line;
class ShadowBox;

// A generic window for the in-game UI.
class Window : public Element{
public:
    static px_t HEADING_HEIGHT;
    static px_t CLOSE_BUTTON_SIZE;

    typedef void (*InitFunction)(Client &);

    static Window *InitializeLater(InitFunction function, const std::string &title);
    static Window *WithRectAndTitle(const ScreenRect &rect, const std::string &title);

    static void hideWindow(void *window);
    const std::string &title() const { return _title; }

    static void startDragging(Element &e, const ScreenPoint &mousePos);
    static void stopDragging(Element &e, const ScreenPoint &mousePos);
    static void drag(Element &e, const ScreenPoint &mousePos);

    void resize(px_t w, px_t h); // Resize window, so that the content size matches the given dims.
    void width(px_t w) override;
    void height(px_t h) override;
    void setTitle(const std::string &title);
    void center();
    
    px_t contentWidth() const { return _content->width(); }
    px_t contentHeight() const { return _content->height(); }
    const Label *getHeading() const { return _heading; }

    void addChild(Element *child) override;
    void clearChildren() override;
    Element *findChild(const std::string id) const override;
    bool isInitialized() const { return _initFunction == nullptr || _isInitialized; }

protected:
    Window();

private:
    void addStructuralElements();
    void addBackground();
    void addHeading();
    void addBorder();
    void addContent();

    std::string _title;
    bool _dragging; // Whether this window is currently being dragged by the mouse.
    ScreenPoint _dragOffset; // While dragging, where the mouse is on the window.
    Element *_content;
    ColorBlock *_background;
    ShadowBox *_border;
    Label *_heading;
    Line *_headingLine;
    Button *_closeButton;

    InitFunction _initFunction; // Called before first draw
    bool _isInitialized;
    static void checkInitialized(Element &thisWindow);
};

#endif
