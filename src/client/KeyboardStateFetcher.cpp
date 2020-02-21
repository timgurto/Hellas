#include "KeyboardStateFetcher.h"

bool KeyboardStateFetcher::isPressed(SDL_Scancode key) const {
  if (_fakeKeysPressed.count(key) == 1) return true;

  const auto *realKeyboardState = SDL_GetKeyboardState(nullptr);
  return realKeyboardState[key] == SDL_PRESSED;
}

void KeyboardStateFetcher::startFakeKeypress(SDL_Scancode key) {
  _fakeKeysPressed.insert(key);
}
