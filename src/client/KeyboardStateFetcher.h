#pragma once

#include <SDL.h>
#include <SDL_scancode.h>

#include <set>

// This class acts as a wrapper around raw SDL checking of the keyboard state.
// It does this so that tests can fake a keypress.  Key events (down, up) can
// easily be faked by adding them to the event queue, but faking the keyboard
// state requires this wrapper level.
//
// For example, movement via the keyboard can be tested only in this way.

class KeyboardStateFetcher {
 public:
  bool isPressed(SDL_Scancode key) const;
  void startFakeKeypress(SDL_Scancode key);

 private:
  std::set<SDL_Scancode> _fakeKeysPressed;
};
