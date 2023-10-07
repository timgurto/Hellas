#include "Client.h"

void Client::initToasts() {
  const auto TOAST_H = 40, TOAST_W = 200;

  auto x = (SCREEN_X - TOAST_W) / 2;
  _toasts = new List({x, 0, TOAST_W, SCREEN_Y}, TOAST_H);
  _toasts->id("Toasts");
  addUI(_toasts);

  populateToastsList();
}

void Client::updateToasts() {
  for (const auto &msg : _queuedToastsFromOtherThreads) toast({msg}, msg);

  _queuedToastsFromOtherThreads.clear();

  // Update all toasts' timers.  If a timer runs out, remove it and repopulate
  // List.

  auto aToastNeedsRemoving = false;

  for (auto &info : toastInfo) {
    if (info.timeRemaining < _timeElapsed) {
      info.timeRemaining = 0;
      aToastNeedsRemoving = true;
    } else
      info.timeRemaining -= _timeElapsed;
  }

  if (!aToastNeedsRemoving) return;

  toastInfo.remove_if(
      [](const Toast &info) { return info.timeRemaining == 0; });

  populateToastsList();
}

void Client::toast(const std::string &icon, const std::string &text) {
  toast(images.icons[icon], text);
}

void Client::toast(const Texture &icon, const std::string &text) {
  Toast t;
  t.icon = icon;
  t.text = text;
  t.timeRemaining = 8000;

  toastInfo.push_back(t);
  populateToastsList();

  _debug(text);
}

void Client::populateToastsList() {
  _toasts->clearChildren();

  for (auto &info : toastInfo) {
    auto toast = new Element();
    _toasts->addChild(toast);
    toast->height(toast->height() - 3);

    // Background
    toast->addChild(new ColorBlock({0, 0, toast->width(), toast->height()},
                                   Element::BACKGROUND_COLOR));
    toast->addChild(new ShadowBox({0, 0, toast->width(), toast->height()}));

    // Icon
    const auto gap = (toast->height() - info.icon.height()) / 2;
    toast->addChild(new Picture(gap, gap, info.icon));

    // Text
    const px_t textX = info.icon.width() + 2 * gap,
               textW = toast->width() - textX - gap;
    static auto wordWrapper = WordWrapper{_defaultFont, textW};
    auto lines = wordWrapper.wrap(info.text);
    auto totalTextHeight = lines.size() * Element::TEXT_HEIGHT;
    auto y = (static_cast<int>(toast->height()) -
              static_cast<int>(totalTextHeight)) /
             2;
    for (const auto &line : lines) {
      toast->addChild(new Label({textX, y, textW, Element::TEXT_HEIGHT}, line,
                                Element::CENTER_JUSTIFIED));
      y += Element::TEXT_HEIGHT;
    }
  }

  _toasts->resizeToContent();
}
