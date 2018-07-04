#include "CQuest.h"
#include "../Rect.h"
#include "Client.h"
#include "ui/Window.h"

void CQuest::generateWindow(void *questAsVoid) {
  const auto WIN_W = 200_px, WIN_H = 200_px;

  auto quest = reinterpret_cast<CQuest *>(questAsVoid);
  auto window = Window::WithRectAndTitle({0, 0, WIN_W, WIN_H}, "Quest");
  window->center();

  const auto BOTTOM = window->contentHeight();
  const auto GAP = 2_px;
  auto y = GAP;

  // Quest name
  auto name = new Label({GAP, y, WIN_W, Element::TEXT_HEIGHT}, quest->name());
  name->setColor(Color::HELP_TEXT_HEADING);
  window->addChild(name);

  // Accept button

  quest->_window = window;
  Client::instance().addWindow(quest->_window);
  quest->_window->show();
}
