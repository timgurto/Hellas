#include "CQuest.h"
#include "../Rect.h"
#include "ui/Window.h"

void CQuest::generateWindow(void *questAsVoid) {
  auto quest = reinterpret_cast<CQuest *>(questAsVoid);
  quest->_window = Window::WithRectAndTitle({20, 20, 100, 100}, "quest");
}
