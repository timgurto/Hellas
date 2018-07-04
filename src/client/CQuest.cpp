#include "CQuest.h"
#include "../Rect.h"
#include "Client.h"
#include "ui/Window.h"

void CQuest::generateWindow(void *questAsVoid) {
  auto quest = reinterpret_cast<CQuest *>(questAsVoid);
  quest->_window = Window::WithRectAndTitle({20, 20, 100, 100}, "quest");
  Client::instance().addWindow(quest->_window);
  quest->_window->show();
}
