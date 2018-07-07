#pragma once

void initialiseSDL();
void finaliseSDL();

void handleInput(unsigned timeElapsed);
void render();

template <typename T>
T min(const T &lhs, const T &rhs) {
  return lhs < rhs ? lhs : rhs;
}

enum Direction { UP, DOWN, LEFT, RIGHT };
void pan(Direction dir);

void enforcePanLimits();
