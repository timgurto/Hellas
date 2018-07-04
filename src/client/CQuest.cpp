#include "CQuest.h"
#include "../Rect.h"
#include "CQuest.h"
#include "Client.h"
#include "ui/Window.h"

void CQuest::generateWindow(CQuest *quest, size_t startObjectSerial) {
  const auto WIN_W = 200_px, WIN_H = 200_px;

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
      new Button({GAP, BUTTON_Y, BUTTON_W, BUTTON_H}, "Accept quest",
                 [=]() { acceptQuest(quest, startObjectSerial); });
  acceptButton->id("accept");
  window->addChild(acceptButton);

  quest->_window = window;
  Client::instance().addWindow(quest->_window);
  quest->_window->show();
}

void CQuest::acceptQuest(CQuest *quest, size_t startObjectSerial) {
  auto &client = Client::instance();

  // Send message
  client.sendMessage(CL_ACCEPT_QUEST, makeArgs(quest->id(), startObjectSerial));

  // Close and remove window
  quest->_window->hide();
  // TODO: better cleanup.  Lots of unused windows in the background may take up
  // significant memory.  Note that this function is called from a button click
  // (which subsequently changes the appearance of the button), meaning it is
  // unsafe to delete the window here.
}
