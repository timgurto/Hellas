#include "CQuest.h"
#include "../Rect.h"
#include "ui/Window.h"

void CQuest::generateWindow() {
  _window = Window::WithRectAndTitle({20, 20, 100, 100}, "quest");
}
