#pragma once

#include <set>
#include <string>

void initialiseSDL();
void finaliseSDL();

using FilesList = std::set<std::string>;
FilesList findDataFiles(const std::string &path);

void handleInput(unsigned timeElapsed);
void render();

enum Direction { UP, DOWN, LEFT, RIGHT };
void pan(Direction dir);

void enforcePanLimits();

void zoomIn();
void zoomOut();

int zoomed(int value);
