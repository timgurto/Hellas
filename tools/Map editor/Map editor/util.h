#pragma once

#include <set>

#include "../../../src/Point.h"

void drawCircle(ScreenPoint &p, int radius);

using FilesList = std::set<std::string>;
FilesList findDataFiles(const std::string &path);
