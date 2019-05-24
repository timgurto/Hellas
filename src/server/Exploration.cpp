#include "Exploration.h"

#include "../XmlReader.h"
#include "../XmlWriter.h"
#include "Server.h"

Exploration::Exploration(size_t mapWidth, size_t mapHeight) {
  size_t chunksX = mapWidth / CHUNK_SIZE;
  size_t chunksY = mapHeight / CHUNK_SIZE;
  _map = {chunksX, std::vector<bool>(chunksY, false)};
}

void Exploration::writeTo(XmlWriter &xw) const {
  auto e = xw.addChild("mapExploration");
  auto chunksX = _map.size();
  for (auto x = 0; x != chunksX; ++x) {
    auto data = ""s;
    for (auto y = 0; y != _map[x].size(); ++y) {
      data.push_back(_map[x][y] ? ' ' : 'X');
    }
    auto col = xw.addChild("col", e);
    xw.setAttr(col, "data", data);
  }
}

void Exploration::readFrom(XmlReader &xr) {
  auto e = xr.findChild("mapExploration");
  if (!e) return;
  auto colIndex = 0;
  _numChunksExplored = 0;
  for (auto col : xr.getChildren("col", e)) {
    auto data = ""s;
    if (!xr.findAttr(col, "data", data)) continue;

    auto colV = std::vector<bool>(data.size(), false);
    for (auto i = 0; i != data.size(); ++i) {
      if (data[i] == ' ') {
        colV[i] = true;
        ++_numChunksExplored;
      }
    }
    _map[colIndex] = colV;

    ++colIndex;
  }
}

void Exploration::sendWholeMap(const Socket &socket) const {
  // One message per column
  static const auto NUMBERS_PER_MESSAGE = 10;
  static const auto CHUNKS_PER_NUMBER = 30;

  for (auto col = 0; col != _map.size(); ++col) {
    auto args = makeArgs(col);
    for (auto numberIndex = 0; numberIndex != NUMBERS_PER_MESSAGE;
         ++numberIndex) {
      Uint32 number = 0;
      for (auto i = 0; i != CHUNKS_PER_NUMBER; ++i) {
        number = number << 1;
        size_t y = numberIndex * CHUNKS_PER_NUMBER + i;
        if (y >= _map[col].size()) continue;
        if (_map[col][y]) ++number;
      }

      args = makeArgs(args, number);
    }
    Server::instance().sendMessage(socket, SV_MAP_EXPLORATION_DATA, args);
  }
}

void Exploration::sendSingleChunk(const Socket &socket,
                                  const Chunk &chunk) const {
  Server::instance().sendMessage(socket, SV_CHUNK_EXPLORED,
                                 makeArgs(chunk.x, chunk.y));
}

Exploration::Chunk Exploration::getChunk(const MapPoint &location) {
  auto &map = Server::instance().map();
  auto row = map.getRow(location.y);
  auto col = map.getCol(location.x, row);
  return {col / CHUNK_SIZE, row / CHUNK_SIZE};
}

std::set<Exploration::Chunk> Exploration::explore(const Chunk &chunk) {
  if (_map.empty() || _map.front().empty()) return {};

  auto newlyExploredChunks = std::set<Chunk>{};

  // 3x3 around central chunk
  auto minX = chunk.x > 0 ? chunk.x - 1 : 0,
       minY = chunk.y > 0 ? chunk.y - 1 : 0,
       maxX = min(chunk.x + 1, _map.size() - 1),
       maxY = min(chunk.y + 1, _map.front().size() - 1);
  for (auto x = minX; x <= maxX; ++x)
    for (auto y = minY; y <= maxY; ++y) {
      auto &isChunkExplored = _map[x][y];
      if (isChunkExplored) continue;

      isChunkExplored = true;
      newlyExploredChunks.insert(Chunk{x, y});
    }

  return newlyExploredChunks;
}

bool operator<(const Exploration::Chunk &lhs, const Exploration::Chunk &rhs) {
  if (lhs.x != rhs.x) return lhs.x < rhs.x;
  return lhs.y < rhs.y;
}
