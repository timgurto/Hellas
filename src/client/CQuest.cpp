#include "CQuest.h"
#include "../Rect.h"
#include "CQuest.h"
#include "Client.h"
#include "WordWrapper.h"
#include "ui/Line.h"
#include "ui/Window.h"

void CQuest::generateWindow(CQuest *quest, size_t startObjectSerial,
                            Transition pendingTransition) {
  const auto WIN_W = 200_px, WIN_H = 200_px;

  auto window = Window::WithRectAndTitle({0, 0, WIN_W, WIN_H}, "Quest");
  window->center();

  const auto BOTTOM = window->contentHeight();
  const auto GAP = 2_px, BUTTON_W = 90_px, BUTTON_H = 16_px,
             CONTENT_W = WIN_W - 2 * GAP;
  auto y = GAP;

  // Quest name
  auto name = new Label({GAP, y, WIN_W, Element::TEXT_HEIGHT}, quest->name());
  name->setColor(Color::HELP_TEXT_HEADING);
  window->addChild(name);
  y += name->height() + GAP;

  // Body
  const auto BODY_H = BOTTOM - 2 * GAP - BUTTON_H - y;
  auto body = new List({GAP, y, CONTENT_W, BODY_H});
  window->addChild(body);
  auto ww = WordWrapper{Element::font(), body->contentWidth()};
  auto lines = ww.wrap(quest->_brief);
  for (const auto &line : lines) body->addChild(new Label({}, line));

  y += BODY_H + GAP;

  // Transition button
  const auto TRANSITION_BUTTON_RECT = ScreenRect{GAP, y, BUTTON_W, BUTTON_H};
  auto transitionName =
      pendingTransition == ACCEPT ? "Accept quest"s : "Complete quest"s;
  auto transitionFun =
      pendingTransition == ACCEPT ? acceptQuest : completeQuest;
  Button *transitionButton =
      new Button(TRANSITION_BUTTON_RECT, transitionName,
                 [=]() { transitionFun(quest, startObjectSerial); });
  ;
  window->addChild(transitionButton);

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
  // TODO: better cleanup.  Lots of unused windows in the background may take
  // up significant memory.  Note that this function is called from a button
  // click (which subsequently changes the appearance of the button), meaning
  // it is unsafe to delete the window here.
}

void CQuest::completeQuest(CQuest *quest, size_t startObjectSerial) {
  auto &client = Client::instance();

  // Send message
  client.sendMessage(CL_COMPLETE_QUEST,
                     makeArgs(quest->id(), startObjectSerial));

  // Close and remove window
  quest->_window->hide();
  // TODO: better cleanup.  Lots of unused windows in the background may take
  // up significant memory.  Note that this function is called from a button
  // click (which subsequently changes the appearance of the button), meaning
  // it is unsafe to delete the window here.
}
