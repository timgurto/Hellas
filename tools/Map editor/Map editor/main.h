#pragma once

#include <string>

#include "../../../src/Color.h"
#include "../../../src/Point.h"
#include "../../../src/client/Texture.h"

void handleInput(unsigned timeElapsed);
void render();

enum Direction { UP, DOWN, LEFT, RIGHT };
void pan(Direction dir);

void enforcePanLimits();

void zoomIn();
void zoomOut();

int zoomed(int value);
int unzoomed(int value);
double zoomed(double value);
double unzoomed(double value);

ScreenPoint transform(MapPoint mp);

void drawPointOnMap(const MapPoint &mapLoc, Color color);
void drawTextOnMap(const MapPoint &mapLoc, Color color,
                   const std::string &text);
void drawCircleOnMap(const MapPoint &mapLoc, Color color, int radius);
void drawImageOnMap(const MapPoint &mapLoc, const Texture &image,
                    const ScreenRect &drawRect);
void drawRectOnMap(const MapPoint &mapLoc, Color color,
                   const ScreenRect &drawRect);

void initUI();

MapPoint rounded(const MapPoint &rhs);
