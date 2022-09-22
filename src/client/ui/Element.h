#include <functional>
#include <list>

#include "../../Color.h"
#include "../../types.h"
#include "../Renderer.h"
#include "../Texture.h"
#include "../Tooltip.h"

#ifndef ELEMENT_H
#define ELEMENT_H

class Client;

/*
A UI element, making up part of a Window.
The base class may be used as an invisible container for other Elements, to
improve the efficiency of collision detection.
*/
class Element {
 public:
  enum Justification {
    LEFT_JUSTIFIED,
    RIGHT_JUSTIFIED,
    TOP_JUSTIFIED,
    BOTTOM_JUSTIFIED,
    CENTER_JUSTIFIED
  };
  enum Orientation { HORIZONTAL, VERTICAL };
  typedef std::list<Element *> children_t;
  static px_t TEXT_HEIGHT, ITEM_HEIGHT;

  static Color BACKGROUND_COLOR, SHADOW_LIGHT, SHADOW_DARK, FONT_COLOR;

 private:
  static TTF_Font *_font;

  static Texture transparentBackground;

  // The current moused-over element.  Draw its tooltip.
  static const Element *_currentTooltipElement;

  mutable bool _changed{true};     // If true, this element should be refreshed
                                   // before next being drawn.
  bool _dimensionsChanged{false};  // If true, destroy and recreate texture
                                   // before next draw.

  bool _visible{true};

  ScreenRect _rect;  // Location and dimensions within window

  Element *_parent{nullptr};  // nullptr if no parent.

  std::string _id;  // Optional ID for finding children.

  // In case the below needs to point to something new.
  Tooltip _localCopyOfTooltip{};
  const Tooltip *_tooltip{nullptr};  // Optional tooltip

  Uint8 _alpha{SDL_ALPHA_OPAQUE};

  bool _ignoreMouseEvents{false};  // Affects only top-level UI elements.

  static bool initialized;

 protected:
  children_t _children;

  Texture _texture;  // A memoized image of the element, redrawn only when
                     // necessary.

  Client *_client{nullptr};

  virtual void
  checkIfChanged();  // Allows elements to update their changed status.

  static void resetTooltip();  // To be called once, when the mouse moves.

  using mouseDownFunction_t =
      std::function<void(Element &e, const ScreenPoint &mousePos)>;
  using mouseUpFunction_t =
      std::function<void(Element &e, const ScreenPoint &mousePos)>;
  typedef void (*mouseMoveFunction_t)(Element &e, const ScreenPoint &mousePos);
  typedef void (*scrollUpFunction_t)(Element &e);
  typedef void (*scrollDownFunction_t)(Element &e);
  typedef void (*preRefreshFunction_t)(Element &e);

  mouseDownFunction_t _leftMouseDown{nullptr};
  Element *_leftMouseDownElement{nullptr};
  mouseUpFunction_t _leftMouseUp{nullptr};
  Element *_leftMouseUpElement{nullptr};
  mouseDownFunction_t _rightMouseDown{nullptr};
  Element *_rightMouseDownElement{nullptr};
  mouseUpFunction_t _rightMouseUp{nullptr};
  Element *_rightMouseUpElement{nullptr};

  mouseMoveFunction_t _mouseMove{nullptr};
  Element *_mouseMoveElement{nullptr};

  scrollUpFunction_t _scrollUp{nullptr};
  Element *_scrollUpElement{nullptr};
  scrollUpFunction_t _scrollDown{nullptr};
  Element *_scrollDownElement{nullptr};

  preRefreshFunction_t _preRefresh{nullptr};
  Element *_preRefreshElement{nullptr};

  /*
  Perform any extra redrawing.  The renderer can be used directly.
  After this function is called, the element's children are drawn on top.
  */
  virtual void refresh() {}

  void drawChildren() const;

 public:
  Element(const ScreenRect &rect = {});
  virtual ~Element();

  static void initialize();
  static void createTransparentBackground();

  Client *client() const { return _client; }

  static px_t textOffset;

  bool visible() const { return _visible; }
  static TTF_Font *font() { return _font; }
  static void font(TTF_Font *newFont) { _font = newFont; }
  const ScreenRect &rect() const { return _rect; }
  ScreenPoint position() const { return {_rect.x, _rect.y}; }
  void setPosition(px_t x, px_t y);
  void rect(const ScreenRect &rhs);
  void rect(px_t x, px_t y, px_t w, px_t h);
  px_t width() const { return _rect.w; }
  virtual void width(px_t w);
  px_t height() const { return _rect.h; }
  virtual void height(px_t h);
  ScreenRect rectToFill();
  bool changed() const { return _changed; }
  const std::string &id() const { return _id; }
  void id(const std::string &id) { _id = id; }
  const children_t &children() const { return _children; }
  void setTooltip(const std::string &text);  // Add a simple tooltip.
  void setTooltip(const char *text) { setTooltip(std::string(text)); }
  void setTooltip(const Tooltip &tooltip);  // Add a complex tooltip.
  void clearTooltip();
  static const Tooltip *tooltip();
  const Element *parent() const { return _parent; }
  const Texture &texture() const { return _texture; }
  static bool isInitialized() { return initialized; }
  void setAlpha(Uint8 alpha);
  void ignoreMouseEvents() { _ignoreMouseEvents = true; }
  bool isIgnoringMouseEvents() const { return _ignoreMouseEvents; }
  bool canReceiveMouseEvents() const { return !_ignoreMouseEvents; }
  void setClient(Client &c) { _client = &c; }

  void show();
  void hide();
  void toggleVisibility();
  static void toggleVisibilityOf(void *element);

  void markChanged() const;

  virtual void addChild(Element *child);
  virtual void clearChildren();  // Delete all children
  virtual Element *findChild(const std::string id)
      const;  // Find a child by ID, or nullptr if not found.

  // To be called during refresh.
  void makeBackgroundTransparent();

  // e: allows the function to be called on behalf of another element.  nullptr
  // = self.
  void setLeftMouseDownFunction(mouseDownFunction_t f, Element *e = nullptr);
  void setLeftMouseUpFunction(mouseUpFunction_t f, Element *e = nullptr);
  void setRightMouseDownFunction(mouseDownFunction_t f, Element *e = nullptr);
  void setRightMouseUpFunction(mouseUpFunction_t f, Element *e = nullptr);
  void setMouseMoveFunction(mouseMoveFunction_t f, Element *e = nullptr);
  void setScrollUpFunction(scrollUpFunction_t f, Element *e = nullptr);
  void setScrollDownFunction(scrollDownFunction_t f, Element *e = nullptr);
  void setPreRefreshFunction(preRefreshFunction_t f, Element *e = nullptr);

  /*
  Recurse to all children, calling _mouseDown() etc. in the lowest element that
  the mouse is over. Return value: whether this or any child has called
  _mouseDown().
  */
  bool onLeftMouseDown(const ScreenPoint &mousePos);
  bool onRightMouseDown(const ScreenPoint &mousePos);
  bool onScrollUp(const ScreenPoint &mousePos);
  bool onScrollDown(const ScreenPoint &mousePos);
  // Recurse to all children, calling _mouse*() in all found.
  bool onLeftMouseUp(const ScreenPoint &mousePos);
  bool onRightMouseUp(const ScreenPoint &mousePos);
  void onMouseMove(const ScreenPoint &mousePos);

  void draw();          // Draw the existing texture to its designated location.
  void forceRefresh();  // Mark this and all children as changed
};

#endif
