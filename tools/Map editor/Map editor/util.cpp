#include "util.h"

#include <windows.h>

#include <algorithm>

#include "../../../src/client/Renderer.h"

extern Renderer renderer;

// From
// https://stackoverflow.com/questions/38334081/howto-draw-circles-arcs-and-vector-graphics-in-sdl
void drawCircle(ScreenPoint &p, int radius) {
  typedef int32_t s32;
  s32 x = radius - 1;
  s32 y = 0;
  s32 tx = 1;
  s32 ty = 1;
  s32 err = tx - (radius << 1);  // shifting bits left by 1 effectively
                                 // doubles the value. == tx - diameter
  while (x >= y) {
    //  Each of the following renders an octant of the circle
    SDL_RenderDrawPoint(renderer.raw(), p.x + x, p.y - y);
    SDL_RenderDrawPoint(renderer.raw(), p.x + x, p.y + y);
    SDL_RenderDrawPoint(renderer.raw(), p.x - x, p.y - y);
    SDL_RenderDrawPoint(renderer.raw(), p.x - x, p.y + y);
    SDL_RenderDrawPoint(renderer.raw(), p.x + y, p.y - x);
    SDL_RenderDrawPoint(renderer.raw(), p.x + y, p.y + x);
    SDL_RenderDrawPoint(renderer.raw(), p.x - y, p.y - x);
    SDL_RenderDrawPoint(renderer.raw(), p.x - y, p.y + x);

    if (err <= 0) {
      y++;
      err += ty;
      ty += 2;
    } else if (err > 0) {
      x--;
      tx += 2;
      err += tx - (radius << 1);
    }
  }
}

FilesList findDataFiles(const std::string &searchPath) {
  auto list = FilesList{};

  WIN32_FIND_DATA fd;
  auto path = std::string{searchPath.begin(), searchPath.end()} + "/";
  std::replace(path.begin(), path.end(), '/', '\\');
  std::string filter = path + "*.xml";
  path.c_str();
  HANDLE hFind = FindFirstFile(filter.c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (fd.cFileName == std::string{"map.xml"}) continue;
      auto file = path + fd.cFileName;
      list.insert(file);
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);
  }

  return list;
}

std::string toPascal(std::string s) {
  if (s.empty()) return s;

  s[0] = toupper(s[0]);
  for (auto i = size_t{1}; i < s.size(); ++i) s[i] = tolower(s[i]);

  return s;
}
