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
  const auto BUTTON_W = 80_px, BUTTON_H = 20_px,
             BUTTON_Y = BOTTOM - GAP - BUTTON_H;
  auto acceptButton =
      new Button({GAP, BUTTON_Y, BUTTON_W, BUTTON_H}, "Accept quest");
  acceptButton->id("accept");
  window->addChild(acceptButton);

  quest->_window = window;
  Client::instance().addWindow(quest->_window);
  quest->_window->show();
}
