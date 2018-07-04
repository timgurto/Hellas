#pragma once

#include <functional>
#include <string>

#include "../../Point.h"
#include "ColorBlock.h"
#include "Element.h"
#include "Label.h"
#include "ShadowBox.h"

// A button which can be clicked, showing visible feedback and performing a
// function.
class Button : public Element {
 public:
  using clickFun_t = std::function<void()>;

  void depress();
  void release(bool click);  // click: whether, on release, the button's
                             // _clickFun will be called.

 private:
  Element *_content{new Element{}};
  ColorBlock *_background{new ColorBlock{{1, 1, 0, 0}}};
  ShadowBox *_border{new ShadowBox{{}}};
  Label *_caption{nullptr};

  clickFun_t _clickFun{[]() {}};

  bool _mouseButtonDown{false};
  bool _depressed{false};

  bool _enabled{true};  // False: greyed out; can't be clicked.

  static void mouseDown(Element &e, const ScreenPoint &mousePos);
  static void mouseUp(Element &e, const ScreenPoint &mousePos);
  static void mouseMove(Element &e, const ScreenPoint &mousePos);

 public:
  Button(const ScreenRect &rect, const std::string &caption = "",
         clickFun_t clickFunction = []() {});
  virtual void addChild(Element *child) override;
  virtual void clearChildren() override;
  virtual Element *findChild(const std::string id) const override;

  void clickFun(const clickFun_t &fun) { _clickFun = fun; }

  void width(px_t w) override;
  void height(px_t h) override;

  void enable();
  void disable();

 private:
  void init(const std::string &caption);
};
