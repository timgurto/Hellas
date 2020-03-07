#include "Map.h"

#include "util.h"

void Map::loadFromXML(XmlReader &xr) {
  auto elem = xr.findChild("size");
  if (elem == nullptr || !xr.findAttr(elem, "x", _w) ||
      !xr.findAttr(elem, "y", _h)) {
    return;
  }

  _grid = std::vector<std::vector<char> >(_w);
  for (auto &col : _grid) {
    col = std::vector<char>(_h, '\0');
  }

  for (auto row : xr.getChildren("row")) {
    size_t y;
    if (!xr.findAttr(row, "y", y) || y >= _h) break;
    std::string rowTerrain;
    if (!xr.findAttr(row, "terrain", rowTerrain)) break;
    for (size_t x = 0; x != rowTerrain.size(); ++x) {
      if (x > _w) break;
      _grid[x][y] = rowTerrain[x];
    }
  }
}

size_t Map::getRow(double yCoord) const {
  if (yCoord < 0) return 0;

  size_t row = static_cast<size_t>(yCoord / TILE_H);
  if (row >= _h) row = _h - 1;

  return row;
}

size_t Map::getCol(double xCoord, size_t row) const {
  if (xCoord < 0) return 0;

  double originalX = xCoord;
  if (row % 2 == 1) xCoord += TILE_W / 2;
  auto col = static_cast<size_t>(xCoord / TILE_W);
  if (col >= _w) col = _w - 1;

  return col;
}

MapRect Map::getTileRect(size_t x, size_t y) {
  MapRect r(static_cast<px_t>(x * TILE_W), static_cast<px_t>(y * TILE_H),
            TILE_W, TILE_H);
  if (y % 2 == 0) r.x -= TILE_W / 2;
  return r;
}

std::set<char> Map::terrainTypesOverlapping(const MapRect &rect,
                                            double extraRadius) const {
  if (extraRadius < 0) extraRadius = 0;

  const double left = max(0.0, rect.x - extraRadius),
               right = rect.x + rect.w + extraRadius,
               top = max(0.0, rect.y - extraRadius),
               bottom = rect.y + rect.h + extraRadius;
  auto tileTop = getRow(top), tileBottom = getRow(bottom);
  if (tileBottom < tileTop) return {};

  std::set<char> tilesInRect;

  // Single row
  if (tileTop == tileBottom) {
    size_t tileLeft = getCol(rect.x, tileTop),
           tileRight = getCol(right, tileTop);
    for (size_t x = tileLeft; x <= tileRight; ++x)
      tilesInRect.insert(_grid[x][tileTop]);

    // General case
  } else {
    size_t tileLeftEven = getCol(left, 0), tileLeftOdd = getCol(left, 1),
           tileRightEven = getCol(right, 0), tileRightOdd = getCol(right, 1);
    for (size_t y = tileTop; y <= tileBottom; ++y) {
      bool yIsEven = y % 2 == 0;
      size_t tileLeft = yIsEven ? tileLeftEven : tileLeftOdd,
             tileRight = yIsEven ? tileRightEven : tileRightOdd;
      if (tileRight < tileLeft) return tilesInRect;

      for (size_t x = tileLeft; x <= tileRight; ++x) {
        char terrainIndex = _grid[x][y];
        // Exclude if outside radius
        if (extraRadius != 0) {
          if (tilesInRect.find(terrainIndex) != tilesInRect.end()) continue;
          if (distance(rect, getTileRect(x, y)) > extraRadius) continue;
        }
        tilesInRect.insert(terrainIndex);
      }
    }
  }
  return tilesInRect;
}

char Map::getTerrainAtPoint(const MapPoint &p) const {
  auto row = getRow(p.y);
  auto col = getCol(p.x, row);
  return _grid[col][row];
}

MapPoint Map::randomPoint() const {
  return {randDouble() * (_w - 0.5) * TILE_W, randDouble() * _h * TILE_H};
}

MapPoint Map::randomPointInTile(size_t x, size_t y) {
  auto tile = getTileRect(x, y);
  return {tile.x + randDouble() * tile.w, tile.y + randDouble() * tile.h};
}
