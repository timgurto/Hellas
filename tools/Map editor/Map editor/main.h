#pragma once

void initialiseSDL();
void finaliseSDL();

void handleInput();
void render();

Color randomColor();

template <typename T>
T min(const T &lhs, const T &rhs) {
  return lhs < rhs ? lhs : rhs;
}
